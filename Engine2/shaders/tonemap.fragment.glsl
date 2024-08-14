#include <shaders/header.glsl>

in vec2 inUv;
out vec4 outColor;

// Push constants
layout(push_constant) uniform ConstantData
{
	vec4 parameters; //whitepoint.rgb, exposure.a
};


layout(set = 0, binding = 0) uniform texture2D hdrTexture;
layout(set = 0, binding = 1) uniform sampler clampSampler;



vec3 Uncharted2Tonemap(vec3 x)
{
  float A = 0.15;
  float B = 0.50;
  float C = 0.10;
  float D = 0.20;
  float E = 0.02;
  float F = 0.30;

  return ((x*(A*x+C*B)+D*E)/(x*(A*x+B)+D*F))-E/F;
}

void main()
{
	vec4 hdr = texture(sampler2D(hdrTexture, clampSampler), inUv).rgba * parameters.w;

    hdr.rgb = Uncharted2Tonemap(hdr.rgb) / Uncharted2Tonemap(parameters.xyz);

	outColor.rgba = hdr;
}