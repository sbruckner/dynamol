#version 450

uniform mat4 modelViewProjectionMatrix;
uniform mat4 inverseModelViewProjectionMatrix;

in vec4 gFragmentPosition;
flat in vec4 gSpherePosition;
flat in float gSphereRadius;
flat in uint gSphereId;

out vec4 fragPosition;
out vec4 fragNormal;

struct Sphere
{			
	bool hit;
	vec3 near;
	vec3 far;
	vec3 normal;
};
																					
Sphere calcSphereIntersection(float r, vec3 origin, vec3 center, vec3 line)
{
	vec3 oc = origin - center;
	vec3 l = normalize(line);
	float loc = dot(l, oc);
	float under_square_root = loc * loc - dot(oc, oc) + r*r;
	if (under_square_root > 0)
	{
		float da = -loc + sqrt(under_square_root);
		float ds = -loc - sqrt(under_square_root);
		vec3 near = origin+min(da, ds) * l;
		vec3 far = origin+max(da, ds) * l;
		vec3 normal = (near - center);

		return Sphere(true, near, far, normal);
	}
	else
	{
		return Sphere(false, vec3(0), vec3(0), vec3(0));
	}
}

float calcDepth(vec3 pos)
{
	float far = gl_DepthRange.far; 
	float near = gl_DepthRange.near;
	vec4 clip_space_pos = modelViewProjectionMatrix * vec4(pos, 1.0);
	float ndc_depth = clip_space_pos.z / clip_space_pos.w;
	return (((far - near) * ndc_depth) + near + far) / 2.0;
}

// based onn code by Inigo Quilez
// https://www.iquilezles.org/www/articles/sphereshadow/sphereshadow.htm

float sphSoftShadow( in vec3 ro, in vec3 rd, in vec4 sph, in float k )
{
    vec3 oc = ro - sph.xyz;
    float b = dot( oc, rd );
    float c = dot( oc, oc ) - sph.w*sph.w;
    float h = b*b - c;
    
    // physically plausible shadow
    float d = sqrt( max(0.0,sph.w*sph.w-h)) - sph.w;
    float t = -b - sqrt( max(h,0.0) );
    return (t<0.0) ? 1.0 : smoothstep(0.0, 1.0, 2.5*k*d/t );

    // cheap but not plausible alternative
    //return (b>0.0) ? step(-0.0001,c) : smoothstep( 0.0, 1.0, h*k/b );
}    
/*
float sphSoftShadow( in vec3 ro, in vec3 rd, in vec4 sph, in float k )
{
	vec3 oc = ro - sph.xyz;
	float b = dot( oc, rd );
	float c = dot( oc, oc ) - sph.w*sph.w;
	float h = b*b - c;
	
	float d = -sph.w + sqrt( max(0.0,sph.w*sph.w-h));
	float t = -b     - sqrt( max(0.0,h) );
	return (t<0.0) ? 1.0 : smoothstep( 0.0, 1.0, k*d/t );
}
*/
/*
float sphSoftShadow( in vec3 ro, in vec3 rd, in vec4 sph, in float k )
{
	vec3 oc = ro - sph.xyz;
	float b = dot( oc, rd );
	float c = dot( oc, oc ) - sph.w*sph.w;

	if (c>gSphereRadius*gSphereRadius)
		discard;

	float h = b*b - c;
	
	return (b>0.0) ? step(-0.0001,c) : smoothstep( 0.0, 1.0, h*k/b );
}
*/


void main()
{
	vec4 fragCoord = gFragmentPosition;
	fragCoord /= fragCoord.w;
	
	vec4 near = inverseModelViewProjectionMatrix*vec4(fragCoord.xy,-1.0,1.0);
	near /= near.w;

	vec4 far = inverseModelViewProjectionMatrix*vec4(fragCoord.xy,1.0,1.0);
	far /= far.w;

	vec3 V = normalize(far.xyz-near.xyz);	

	float d = 1.0-sphSoftShadow(near.xyz,V,vec4(gSpherePosition.xyz,gSphereRadius*0.25),64.0);
	fragPosition = vec4(d,0,0,0);//length(sphere.near.xyz-near.xyz));
	//gl_FragDepth = 0.0;//depth;
}