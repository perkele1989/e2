#version 460 core
#define EPSILON 1e-5

out vec2 outUv;

void main()
{
	outUv = vec2((gl_VertexIndex << 1) & 2, gl_VertexIndex & 2);
	gl_Position = vec4(outUv * 2.0f + -1.0f, 0.0f, 1.0f);
}