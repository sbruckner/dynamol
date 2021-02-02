#version 450
#include "/defines.glsl"
#include "/globals.glsl"

layout(pixel_center_integer) in vec4 gl_FragCoord;

uniform mat4 modelViewMatrix;
uniform mat4 projectionMatrix;
uniform mat4 modelViewProjectionMatrix;
uniform mat4 inverseModelViewProjectionMatrix;
uniform mat3 normalMatrix;
uniform mat3 inverseNormalMatrix;
uniform mat4 modelLightMatrix;
uniform mat4 modelLightProjectionMatrix;

uniform vec3 lightPosition;
uniform vec3 diffuseMaterial;
uniform vec3 ambientMaterial;
uniform vec3 specularMaterial;
uniform float shininess;
uniform vec3 backgroundColor;

uniform sampler2D spherePositionTexture;
uniform sampler2D sphereNormalTexture;
uniform sampler2D sphereDiffuseTexture;
uniform sampler2D surfacePositionTexture;
uniform sampler2D surfaceNormalTexture;
uniform sampler2D surfaceDiffuseTexture;
uniform sampler2D depthTexture;
uniform sampler2D ambientTexture;
uniform sampler2D materialTexture;
uniform sampler2D environmentTexture;
uniform sampler2D shadowColorTexture;
uniform sampler2D shadowDepthTexture;
uniform bool environment;

uniform float maximumCoCRadius = 0.0;
uniform float aparture = 0.0;
uniform float focalDistance = 0.0;
uniform float focalLength = 0.0;

uniform float distanceBlending = 0.0;
uniform float distanceScale = 1.0;

uniform vec3 objectCenter;
uniform float objectRadius;

in vec4 gFragmentPosition;
out vec4 fragColor;

// From http://http.developer.nvidia.com/GPUGems/gpugems_ch17.html
vec2 latlong(vec3 v)
{
	v = normalize(v);
	float theta = acos(v.z) / 2; // +z is up
	float phi = atan(v.y, v.x) + 3.1415926535897932384626433832795;
	return vec2(phi, theta) * vec2(0.1591549, 0.6366198);
}

vec4 over(vec4 vecF, vec4 vecB)
{
	return vecF + (1.0-vecF.a)*vecB;
}

// from https://www.iquilezles.org/www/articles/functions/functions.htm
float cubicPulse( float c, float w, float x )
{
    x = abs(x - c);
    if( x>w ) return 0.0;
    x /= w;
    return 1.0 - x*x*(3.0-2.0*x);
}

// from https://www.iquilezles.org/www/articles/filterableprocedurals/filterableprocedurals.htm
float filteredGrid( in vec2 p, in vec2 dpdx, in vec2 dpdy )
{
    const float N = 32.0;
    vec2 w = max(abs(dpdx), abs(dpdy));
    vec2 a = p + 0.5*w;                        
    vec2 b = p - 0.5*w;           
    vec2 i = (floor(a)+min(fract(a)*N,1.0)-
              floor(b)-min(fract(b)*N,1.0))/(N*w);
    return (1.0-i.x)*(1.0-i.y);
}

// https://www.iquilezles.org/www/articles/morecheckerfiltering/morecheckerfiltering.htm
vec2 p( in vec2 x )
{
    vec2 h = fract(x/2.0)-0.5;
    return x*0.5 + h*(1.0-2.0*abs(h));
}

float checkersGradTriangle( in vec2 uv, in vec2 ddx, in vec2 ddy )
{
    vec2 w = max(abs(ddx), abs(ddy)) + 0.01;    // filter kernel
    vec2 i = (p(uv+w)-2.0*p(uv)+p(uv-w))/(w*w); // analytical integral (triangle filter)
    return 0.5 - 0.5*i.x*i.y;                   // xor pattern
}

void main()
{
	vec4 fragCoord = gFragmentPosition;
	fragCoord /= fragCoord.w;
	
	vec4 near = inverseModelViewProjectionMatrix*vec4(fragCoord.xy,-1.0,1.0);
	near /= near.w;

	vec4 far = inverseModelViewProjectionMatrix*vec4(fragCoord.xy,1.0,1.0);
	far /= far.w;

	vec3 V = normalize(far.xyz-near.xyz);

	vec4 spherePosition = texelFetch(spherePositionTexture,ivec2(gl_FragCoord.xy),0);
	vec4 sphereNormal = texelFetch(sphereNormalTexture,ivec2(gl_FragCoord.xy),0);
	vec4 sphereDiffuse = texelFetch(sphereDiffuseTexture,ivec2(gl_FragCoord.xy),0);

	vec4 surfacePosition = texelFetch(surfacePositionTexture,ivec2(gl_FragCoord.xy),0);
	vec4 surfaceNormal = texelFetch(surfaceNormalTexture,ivec2(gl_FragCoord.xy),0);
	vec4 surfaceDiffuse = texelFetch(surfaceDiffuseTexture,ivec2(gl_FragCoord.xy),0);

	vec3 directLight = vec3(1.0);

	if (surfacePosition.w >= 65535.0)	
		surfaceDiffuse.a = 0.0;
	
	// experimental: ground place (disabled for now)
	/*
	{

		vec3 planeNormal = vec3(0.0,1.0,0.0);
		vec3 planePosition = objectCenter-vec3(0.0,objectRadius,0.0);

		float denom = dot(planeNormal,V);
		float t = dot(planePosition-near.xyz,planeNormal)/denom;

		vec4 planeIntersection;
		planeIntersection.xyz = near.xyz+t*V;
		planeIntersection.w = length(planeIntersection.xyz-near.xyz);

		vec2 gridCoords = planeIntersection.xz/32.0;
		vec2 ddxXZ = dFdx( gridCoords ); 
        vec2 ddyXZ = dFdy( gridCoords); 

		if (surfacePosition.w >= 65535.0)
		{
			surfaceDiffuse.a = 0.0;

			if (abs(denom) > 0.00001)
			{
				if (t >= 0.0)
				{
					vec4 lightSpacePosition = modelLightProjectionMatrix * vec4(planeIntersection.xyz, 1.0);
					lightSpacePosition /= lightSpacePosition.w;
					lightSpacePosition.xyz *= 0.5;
					lightSpacePosition.xyz += 0.5;
					vec2 lightSpaceCoordinates = lightSpacePosition.xy;


					surfacePosition = planeIntersection;
					surfaceNormal.xyz =  normalize(normalMatrix*planeNormal);
				
					const int kernelSize = 2;
					const float kernelNormalization = float(kernelSize * 2 + 1) * float(kernelSize * 2 + 1);

					float shadowValue = 0.0;

					for (int xOffset = -kernelSize; xOffset <= kernelSize; ++xOffset)
					{
						for (int yOffset = -kernelSize; yOffset <= kernelSize; ++yOffset)
						{
							shadowValue += textureOffset(shadowColorTexture, lightSpaceCoordinates,ivec2(xOffset,yOffset)).r;
						}
					}

					shadowValue /= kernelNormalization;
   					float lightValue = clamp(1.0-shadowValue,0.0,1.0);
					directLight = vec3(lightValue);

					float centerDistance = length(planePosition.xyz-planeIntersection.xyz);
					surfaceDiffuse.a =exp(-centerDistance/(4.0*objectRadius));

					float gridValue = 0.5+0.5*checkersGradTriangle(gridCoords,ddxXZ,ddyXZ);
					surfaceDiffuse.rgb = vec3(gridValue);
				}				
			}
		}
	}
	*/
	vec4 background = vec4(backgroundColor,1.0);

#ifdef ENVIRONMENT
	background = textureLod(environmentTexture,latlong(V),0.0);
#endif

	vec4 ambient = vec4(1.0,1.0,1.0,1.0);

#ifdef AMBIENT
	ambient = texelFetch(ambientTexture,ivec2(gl_FragCoord.xy),0);
#endif

	float depth = texelFetch(depthTexture,ivec2(gl_FragCoord.xy),0).x;
	vec3 surfaceNormalWorld = normalize(inverseNormalMatrix*surfaceNormal.xyz);

	vec3 ambientColor = ambientMaterial*ambient.a;
	vec3 diffuseColor = surfaceDiffuse.rgb*diffuseMaterial*ambient.a;
	vec3 specularColor = specularMaterial*ambient.a;

	vec3 N = surfaceNormalWorld;
	vec3 L = normalize(lightPosition-surfacePosition.xyz);
	vec3 R = normalize(reflect(L, N));
	float NdotV = max(0.0,abs(dot(N, V)));
	float NdotL = dot(N, L);
	float RdotV = max(0.0,dot(R, V));
	
	vec3 color = vec3(0.0);
	float light_occlusion = 1.0;

#ifdef AMBIENT
	vec3 VL =  normalize(normalMatrix*normalize(lightPosition-surfacePosition.xyz));
	light_occlusion = 1.0-clamp(dot(VL.xyz, ambient.xyz),0.0,1.0);
#endif

	float lightRadius = 4.0*length(lightPosition);
	float lightDistance = length(lightPosition.xyz-surfacePosition.xyz) / lightRadius;
	float lightAttenuation = 1.0 / (1.0 + lightDistance*lightDistance);
	light_occlusion *= lightAttenuation;

	NdotL = clamp((NdotL+1.0)*0.5,0.0,1.0);
	
#ifdef ENVIRONMENTLIGHTING
	
	//from http://casual-effects.blogspot.com/2011/08/plausible-environment-lighting-in-two.html
	float environmentMapWidth = float(textureSize(environmentTexture,0).x);
	float mipLevel = 0.5 * (log2(environmentMapWidth*environmentMapWidth / (shininess + 1.0)) - 1.0);
 
	vec3 diffuseEnvironmentColor = textureLod(environmentTexture, latlong(N.xyz),32.0).rgb;
	vec3 specularEnvironmentColor = textureLod(environmentTexture, latlong(R.xyz),mipLevel).rgb;

	color = ambientColor + (ambientColor+directLight) * light_occlusion * (NdotL * diffuseColor * diffuseEnvironmentColor + specularEnvironmentColor * specularColor + pow(RdotV,shininess) * specularEnvironmentColor * specularColor);

#else
	
	color = ambientColor + (ambientColor+directLight) * light_occlusion * (NdotL * diffuseColor + pow(RdotV,shininess) * specularColor );

#endif


	color.rgb += distanceBlending*vec3(min(1.0,pow(abs(spherePosition.w-surfacePosition.w),distanceScale)));

	vec4 final = over(vec4(color,1.0)*surfaceDiffuse.a,background);

#ifdef DEPTHOFFIELD
	vec4 cp = modelViewMatrix*vec4(surfacePosition.xyz, 1.0);
	cp = cp / cp.w;
	float dist = length(cp.xyz);

	float coc = maximumCoCRadius * aparture * (focalLength * (focalDistance - dist)) / (dist * (focalDistance - focalLength));
	coc = clamp( coc * 0.5 + 0.5, 0.0, 1.0 );

	if (surfacePosition.w >= 65535.0)	
		coc = 0.0;

	final.a = coc;
#endif

	// experimental: debug shadow map output
	/*		
	if (fragCoord.x < -0.75 && fragCoord.y < -0.75)
	{
		vec4 value = texture(shadowColorTexture,(fragCoord.xy+vec2(1.0))/0.25);
		final = vec4(value.xyz,1.0);
	}
	*/

	fragColor = final; 
}