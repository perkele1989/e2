#version 460 core

in vec4 vertexPosition;
out vec2 fragmentUv;

layout(push_constant) uniform ConstantData
{
    vec2 resolution;
    vec2 areaSize;
    vec2 moveOffset;
    float timeDelta;
};

layout(set = 0, binding = 0) uniform sampler areaSampler;
layout(set = 0, binding = 1) uniform texture2D areaTexture;

void main()
{
    vec2 moveOffsetPixels = (moveOffset / areaSize) * resolution;

	fragmentUv = vertexPosition.xy;
	vec4 vertPos = vertexPosition;
	vertPos.xy = (vertPos.xy * resolution) - moveOffsetPixels;
	vertPos.xy = vertPos.xy * (1.0 / resolution);
	vertPos.xy = vertPos.xy * 2.0 - 1.0; // -1 -> +1
	vertPos.z = 0.0;
	vertPos.w = 1.0;

	gl_Position = vertPos;
}