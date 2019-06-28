// This is an implementation of the depth-of-field method by McGuire.
// It is based on the source code available at the link below, with only slight adaptations.
// 
// Morgan McGuire. The Skylanders SWAP Force Depth-of-Field Shader.
// http://casual-effects.blogspot.com/2013/09/the-skylanders-swap-force-depth-of.html

#version 450

#define saturate(s) clamp( s, 0.0, 1.0 )

layout(pixel_center_integer) in vec4 gl_FragCoord;

uniform sampler2D colorTexture;
uniform sampler2D nearTexture;
uniform sampler2D blurTexture;

uniform float maximumCoCRadius = 0.0;
uniform float aparture = 0.0;
uniform float focalDistance = 0.0;
uniform float focalLength = 0.0;

uniform float farRadiusRescale = 1.0;
out vec4 fragColor;

const vec2 kCocReadScaleBias = vec2(2.0, -1.0);

// Boost the coverage of the near field by this factor.  Should always be >= 1
//
// Make this larger if near-field objects seem too transparent
//
// Make this smaller if an obvious line is visible between the near-field blur and the mid-field sharp region
// when looking at a textured ground plane.
const float kCoverageBoost = 1.5;

void main()
{
	ivec2 position = ivec2(gl_FragCoord.xy);

	vec4 color = texelFetch(colorTexture, position, 0);
	vec4 near = texelFetch(nearTexture, position, 0);
	vec4 blurred = texelFetch(blurTexture, position, 0);

	// Signed, normalized radius of the circle of confusion.
	// |normRadius| == 1.0 corresponds to camera->maxCircleOfConfusionRadiusPixels()
	//float normRadius = clamp(color.a,-1.0,1.0);
	//normRadius = maximumCoCRadius * aparture * (focalLength * (focalDistance - normRadius)) / (normRadius * (focalDistance - focalLength));
	float normRadius = color.a * 2.0 - 1.0;

	// Fix the far field scaling factor so that it remains independent of the 
	// near field settings
	normRadius *= (normRadius < 0.0) ? farRadiusRescale : 1.0;

	// Boost the blur factor
	// normRadius = clamp( normRadius * 2.0, -1.0, 1.0 );

	// Decrease sharp image's contribution rapidly in the near field
	// (which has positive normRadius)
	if (normRadius > 0.1) {
		normRadius = min(normRadius * 1.5, 1.0);
	}

	// Boost the blur factor
	normRadius = clamp( normRadius * 2.0, -1.0, 1.0 );

	if (kCoverageBoost != 1.0)
	{
	    float a = saturate( kCoverageBoost * near.a );
	    near.rgb = near.rgb * ( a / max( near.a, 0.001 ) );
	    near.a = a;
	}

	// Two mixs, the second of which has a premultiplied alpha
	vec3 result = mix(color.rgb, blurred.rgb, abs(normRadius)) * (1.0 - near.a) + near.rgb;
	//result = blurred.rgb;

	fragColor.rgb = result.rgb;
	fragColor.a = 1.0;
}