// This is an implementation of Scalable Ambient Obscurance by McGuire et al.
// It is based on their source code available at the link below, with only slight adaptations.
// 
// Morgan McGuire, Michael Mara, and David Luebke. Scalable Ambient Obscurance.
// Proceedings of ACM SIGGRAPH/Eurographics High-Performance Graphics, pp. 97--103, 2012.
// http://casual-effects.com/research/McGuire2012SAO/

#version 450

const float KERNEL_RADIUS = 5;
  
uniform float sharpness = 32.0;
uniform vec2  offset; // either set x to 1/width or y to 1/height

layout(binding=0) uniform sampler2D normalTexture;
layout(binding=1) uniform sampler2D ambientTexture;

layout(pixel_center_integer) in vec4 gl_FragCoord;
in vec4 gFragmentPosition;
out vec4 fragColor;


//-------------------------------------------------------------------------

vec4 BlurFunction(vec2 uv, float r, vec4 centerNormal, inout float w_total)
{
  vec4 value = texture2D( ambientTexture, uv );
  vec4 normal = texture2D( normalTexture, uv );
  
  const float BlurSigma = float(KERNEL_RADIUS) * 0.5;
  const float BlurFalloff = 1.0 / (2.0*BlurSigma*BlurSigma);
  
  float ddiff = (normal.w - centerNormal.w) * sharpness;
  float w = exp2(-r*r*BlurFalloff - ddiff*ddiff);
  w_total += w;

  return value*w;
}

void main()
{
  vec4  ambient = texelFetch( ambientTexture, ivec2(gl_FragCoord.xy),0);
  vec4  normal = texelFetch( normalTexture, ivec2(gl_FragCoord.xy),0);
  normal.w = min(1.0,normal.w);
  
  vec4 total = ambient;
  float w_total = 1.0;

  vec2 texCoord = (gFragmentPosition.xy+1.0)*0.5;
  
  for (float r = 1; r <= KERNEL_RADIUS; ++r)
  {
    vec2 uv = texCoord + offset * (r+0.5);
    total += BlurFunction(uv, r, normal, w_total);  
  }
  
  for (float r = 1; r <= KERNEL_RADIUS; ++r)
  {
    vec2 uv = texCoord - offset * (r+0.5);
    total += BlurFunction(uv, r, normal, w_total);  
  }
  
  fragColor = total / max(w_total,0.0001);
}