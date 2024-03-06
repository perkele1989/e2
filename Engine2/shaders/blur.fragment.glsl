#version 460 core

in vec2 inUv;
out vec4 outColor;

// Push constants
layout(push_constant) uniform ConstantData
{
	vec2 direction;
};

layout(set = 0, binding = 0) uniform texture2D sourceTexture;
layout(set = 0, binding = 1) uniform sampler sourceSampler;

#include <shaders/common/utils.glsl>

void main()
{
    //sampler2D sam = sampler2D(sourceTexture, sourceSampler);
	outColor = blur9(sourceTexture, sourceSampler, inUv, direction);
}