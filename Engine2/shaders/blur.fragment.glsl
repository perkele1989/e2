#version 460 core

in vec2 inUv;
out vec4 outColor;

// Push constants
layout(push_constant) uniform ConstantData
{
	uvec2 resolution;
	vec2 direction;
};

#include <shaders/common/utils.glsl>

void main()
{
	outColor

	outColor.a = 1.0;
}