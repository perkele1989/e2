#version 460 core

out vec4 outColor;

// Push constants
layout(push_constant) uniform ConstantData
{
    mat4 mvpMatrix;
	vec4 color;
};

void main()
{
	outColor = color;
}