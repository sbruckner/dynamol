#version 450
#extension GL_ARB_shading_language_include : require
#include "/defines.glsl"
#include "/globals.glsl"

layout(pixel_center_integer) in vec4 gl_FragCoord;

uniform mat4 modelViewMatrix;
uniform mat4 projectionMatrix;
uniform mat4 modelViewProjectionMatrix;
uniform mat4 inverseModelViewProjectionMatrix;
uniform mat3 normalMatrix;
uniform float sharpness;
uniform uint coloring;
uniform bool environment;
uniform bool lens;

uniform vec3 lightPosition;
uniform vec3 diffuseMaterial;
uniform vec3 ambientMaterial;
uniform vec3 specularMaterial;
uniform float shininess;
uniform vec2 focusPosition;

uniform sampler2D positionTexture;
uniform sampler2D normalTexture;
uniform sampler2D environmentTexture;
uniform sampler2D bumpTexture;
uniform sampler2D materialTexture;
uniform usampler2D offsetTexture;

in vec4 gFragmentPosition;
out vec4 surfacePosition;
out vec4 surfaceNormal;
out vec4 surfaceDiffuse;
out vec4 sphereDiffuse;

struct Element
{
	vec3 color;
	float radius;
};

struct Residue
{
	vec4 color;
};

struct Chain
{
	vec4 color;
};

layout(std140, binding = 0) uniform elementBlock
{
	Element elements[32];
};

layout(std140, binding = 1) uniform residueBlock
{
	Residue residues[32];
};

layout(std140, binding = 2) uniform chainBlock
{
	Chain chains[64];
};

struct BufferEntry
{
	float near;
	float far;
	vec3 center;
	uint id;
	uint previous;
};

layout(std430, binding = 1) buffer intersectionBuffer
{
	uint count;
	BufferEntry intersections[];
};

layout(std430, binding = 2) buffer statisticsBuffer
{
	uint intersectionCount;
	uint totalPixelCount;
	uint totalEntryCount;
	uint maximumEntryCount;
};

struct Sphere
{			
	bool hit;
	vec3 near;
	vec3 far;
	vec3 normal;
};

float calcDepth(vec3 pos)
{
	float far = gl_DepthRange.far; 
	float near = gl_DepthRange.near;
	vec4 clip_space_pos = modelViewProjectionMatrix * vec4(pos, 1.0);
	float ndc_depth = clip_space_pos.z / clip_space_pos.w;
	return (((far - near) * ndc_depth) + near + far) / 2.0;
}

void main()
{
	uint offset = texelFetch(offsetTexture,ivec2(gl_FragCoord.xy),0).r;

	if (offset == 0)
		discard;

	vec4 position = texelFetch(positionTexture,ivec2(gl_FragCoord.xy),0);
	vec4 normal = texelFetch(normalTexture,ivec2(gl_FragCoord.xy),0);

	vec4 fragCoord = gFragmentPosition;
	fragCoord /= fragCoord.w;
	
	vec4 near = inverseModelViewProjectionMatrix*vec4(fragCoord.xy,-1.0,1.0);
	near /= near.w;

	vec4 far = inverseModelViewProjectionMatrix*vec4(fragCoord.xy,1.0,1.0);
	far /= far.w;

	vec3 V = normalize(far.xyz-near.xyz);

	const uint maxEntries = 128;
	uint entryCount = 0;
	uint indices[maxEntries];

	while (offset > 0)
	{
		indices[entryCount++] = offset;
		offset = intersections[offset].previous;
	}

	if (entryCount == 0)
		discard;

	vec4 closestPosition = position;
	vec3 closestNormal = normal.xyz;

	float sharpnessFactor = 1.0;
	vec3 ambientColor = ambientMaterial;
	vec3 diffuseColor = vec3(1.0,1.0,1.0);
	vec3 specularColor = specularMaterial;
	vec3 diffuseSphereColor = vec3(1.0,1.0,1.0);

#ifdef COLORING
	if (coloring > 0)
	{
		uint id = floatBitsToUint(normal.w);
		uint elementId = bitfieldExtract(id,0,8);
		uint residueId = bitfieldExtract(id,8,8);
		uint chainId = bitfieldExtract(id,16,8);

		if (coloring == 1)
			diffuseColor = elements[elementId].color.rgb;
		else if (coloring == 2)
			diffuseColor = residues[residueId].color.rgb;
		else if (coloring == 3)
			diffuseColor = chains[chainId].color.rgb;

		diffuseSphereColor = diffuseColor;
	}
#endif


#ifdef LENSING
	float focusFactor = 0.0;
	if (lens)
	{
		focusFactor = min(16.0,1.0/(16.0*pow(length((fragCoord.xy-focusPosition)/vec2(0.5625,1.0)),2.0)));
		sharpnessFactor += focusFactor;
		focusFactor = min(1.0,focusFactor);
	}
#endif

	uint startIndex = 0;

	// selection sort
	for(uint currentIndex = 0; currentIndex < entryCount; currentIndex++)
	{
		uint minimumIndex = currentIndex;

		for(uint i = currentIndex+1; i < entryCount; i++)
		{
			if(intersections[indices[i]].near < intersections[indices[minimumIndex]].near)
			{
				minimumIndex = i;					
			}
		}

		if (minimumIndex != currentIndex)
		{
			uint temp = indices[minimumIndex];
			indices[minimumIndex] = indices[currentIndex];
			indices[currentIndex] = temp;
		}

		if (startIndex < currentIndex)
		{
			uint endIndex = currentIndex;

			// if span of overlapping spheres of influence has ended, proceed with intersection testing
			if (currentIndex >= entryCount-1 || intersections[indices[startIndex]].far < intersections[indices[currentIndex]].near)
			{
				// sphere tracing parameters
				const uint maximumSteps = 32; // maximum number of steps
				const float eps = 0.0125; // threshold for detected intersection
				const float omega = 1.2; // over-relaxation factor

				const float s = sharpness*sharpnessFactor;

				uint ii = indices[startIndex+1];
				float nearDistance = intersections[ii].near;
				float farDistance = intersections[indices[endIndex-1]].far;

				float maximumDistance = (farDistance-nearDistance)+1.0;
				float surfaceDistance = 1.0;

				vec4 rayOrigin = vec4(near.xyz+V*nearDistance,nearDistance);
				vec4 rayDirection = vec4(V,1.0);
				vec4 currentPosition;
				
				vec4 candidatePosition = rayOrigin;
				vec3 candidateNormal = vec3(0.0);
				vec3 candidateColor = vec3(0.0);
				float candidateValue = 0.0;

				float minimumDistance = maximumDistance;

				uint currentStep = 0;			
				float t = 0.0;

				while (++currentStep <= maximumSteps && t <= maximumDistance)
				{    
					currentPosition = rayOrigin + rayDirection*t;

					if (currentPosition.w > closestPosition.w)
						break;

					float sumValue = 0.0;
					vec3 sumNormal = vec3(0.0);
					vec3 sumColor = vec3(0.0);
					
					// sum contributions of atoms in the neighborhood
					for (uint j = startIndex; j <= endIndex; j++)
					{
						uint ij = indices[j];
						uint id = intersections[ij].id;
						uint elementId = bitfieldExtract(id,0,8);

						vec3 aj = intersections[ij].center;
						float rj = elements[elementId].radius;

						vec3 atomOffset = currentPosition.xyz-aj;
						float atomDistance = length(atomOffset)/rj;

						float atomValue = exp(-s*atomDistance*atomDistance);
						vec3 atomNormal = atomValue*normalize(atomOffset);
						
						sumValue += atomValue;
						sumNormal += atomNormal;

#ifdef COLORING
						vec3 cj = vec3(1.0,1.0,1.0);

						if (coloring == 1)
						{
							cj = elements[elementId].color.rgb;
						}
						else if (coloring == 2)
						{
							uint residueId = bitfieldExtract(id,8,8);
							cj = residues[residueId].color.rgb;
						}
						else if (coloring == 3)
						{
							uint chainId = bitfieldExtract(id,16,8);
							cj = chains[chainId].color.rgb;
						}
#ifdef LENSING
						cj = mix(vec3(diffuseMaterial),cj,focusFactor);
#endif

						vec3 atomColor = cj*atomValue;

						if (coloring > 0)
							sumColor += atomColor;
#endif
					}
					
					surfaceDistance = sqrt(-log(sumValue) / (s))-1.0;

					if (surfaceDistance < eps)
					{
						if (currentPosition.w <= closestPosition.w)
						{
							closestPosition = currentPosition;
							closestNormal = sumNormal;

#ifdef COLORING
							if (coloring > 0)
								diffuseColor = sumColor / sumValue;
#endif
						}
						break;
					}

					if (surfaceDistance < minimumDistance)
					{
						minimumDistance = surfaceDistance;
						candidatePosition = currentPosition;
						candidateNormal = sumNormal;
						candidateColor = sumColor;
						candidateValue = sumValue;
					}

					// Over-relaxation according to the approach described by Keinert et al.
					// However, we simply skip overstepping correction, since it is basically invisible.
					// Benjamin Keinert, Henry Schäfer, Johann Korndörfer, Urs Ganse, and Marc Stamminger.
					// Enhanced Sphere Tracing. Proceedings of Smart Tools and Apps for Graphics (Eurographics Italian Chapter Conference), pp. 1--8, 2014. 
					// http://dx.doi.org/10.2312/stag.20141233
					t += surfaceDistance*omega;
				}
				
				if (currentStep > maximumSteps)
				{
					if (candidatePosition.w <= closestPosition.w)
					{
						closestPosition = candidatePosition;
						closestNormal = candidateNormal;

#ifdef COLORING
						if (coloring > 0)
							diffuseColor = candidateColor / candidateValue;
#endif

					}
				}

				startIndex++;
			}
		}
	}

	if (closestPosition.w >= 65535.0f)
		discard;

#ifdef NORMAL		
	vec3 N = normalize(closestNormal);
	
	// https://medium.com/@bgolus/normal-mapping-for-a-triplanar-shader-10bf39dca05a
	vec3 blend = abs( N );
	blend = normalize(max(blend, 0.00001)); // Force weights to sum to 1.0
	float b = (blend.x + blend.y + blend.z);
	blend /= vec3(b, b, b);	
	
	vec2 uvX = closestPosition.zy*0.5;
	vec2 uvY = closestPosition.xz*0.5;
	vec2 uvZ = closestPosition.xy*0.5;

	vec3 normalX = 2.0*texture(bumpTexture,uvX).xyz - 1.0;
	vec3 normalY = 2.0*texture(bumpTexture,uvY).xyz - 1.0;
	vec3 normalZ = 2.0*texture(bumpTexture,uvZ).xyz - 1.0;

	normalX = vec3(0.0, normalX.yx);
	normalY = vec3(normalY.x, 0.0, normalY.y);
	normalZ = vec3(normalZ.xy, 0.0);

	vec3 worldNormal = normalize(N + normalX.xyz * blend.x + normalY.xyz * blend.y + normalZ.xyz * blend.z);

	closestNormal = worldNormal;
#endif

	vec4 cp = modelViewMatrix*vec4(closestPosition.xyz, 1.0);
	cp = cp / cp.w;

	surfacePosition = closestPosition;

	closestNormal.xyz = normalMatrix*closestNormal.xyz;
	closestNormal.xyz = normalize(closestNormal.xyz);
	surfaceNormal = vec4(closestNormal.xyz,cp.z);

#ifdef MATERIAL
	vec3 materialColor = texture( materialTexture , closestNormal.xy*0.5+0.5 ).rgb;
	diffuseColor *= materialColor;
#endif

	surfaceDiffuse = vec4(diffuseColor,1.0);
	sphereDiffuse = vec4(diffuseSphereColor,1.0);
	gl_FragDepth = calcDepth(closestPosition.xyz);
}