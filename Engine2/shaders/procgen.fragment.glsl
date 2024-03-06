#version 460 core
#define EPSILON 1e-5

in vec2 inUv;
out vec4 outColor;

// Push constants
layout(push_constant) uniform ConstantData
{
	uvec2 resolution;
	float zoom;

	float param1;
	float param2;
	float param3;
	float param4;

	float param5;
	float param6;
	float param7;
	float param8;

	float param9;
	float param10;
	float param11;
	float param12;

	float param13;
	float param14;
	float param15;
	float param16;
};

#include <shaders/common/utils.glsl>

void main()
{
	vec2 position = inUv * zoom;

//	float h = pow(sampleSimplex(position, 0.05), 0.5);
	


	float h = sampleBaseHeight(position);
	//h = h3;
	float m = 0.0;
	float g = 0.0;
	float s = 0.0;
	float o = 0.0;

	if(h > 0.81)
		m = 1.0;
	else if(h > 0.39)
		g = 1.0;
	else if (h > 0.03)
		s = 1.0;
	else 
		o = 1.0;

	vec3 m_C = vec3(0.25, 0.255, 0.26);
	vec3 g_C = vec3(0.2, 0.6, 0.05);
	vec3 s_C = vec3(0.0, 0.1, 0.6);
	vec3 o_C = vec3(0.0, 0.05, 0.3);


	outColor.rgb = o_C;
	outColor.rgb = mix(outColor.rgb, s_C, s);
	outColor.rgb = mix(outColor.rgb, g_C, g);
	outColor.rgb = mix(outColor.rgb, m_C, m);

	//outColor.r = h;

	outColor.a = 1.0;
}