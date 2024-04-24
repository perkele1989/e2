#include <shaders/header.glsl>

out vec4 outColor;

// Push constants
layout(push_constant) uniform ConstantData
{
    mat4 mvpMatrix;
	vec4 visibility;
};

void main()
{
	outColor = visibility;
}