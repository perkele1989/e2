#version 460 core

out vec4 outColor;

// Push constants
layout(push_constant) uniform ConstantData
{
    mat4 mvpMatrix;
	vec3 visibility;
};

void main()
{
	outColor = vec4(visibility, 1.0);
}