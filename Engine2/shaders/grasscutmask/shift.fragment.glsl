
#version 460 core

in vec2 fragmentUv;
out vec4 outColor;

layout(push_constant) uniform ConstantData
{
    vec2 resolution;
    vec2 areaSize;
    vec2 moveOffset;
};

layout(set = 0, binding = 0) uniform sampler areaSampler;
layout(set = 0, binding = 1) uniform texture2D areaTexture;

void main()
{
    float oldHeight = texture(sampler2D(areaTexture, areaSampler), fragmentUv).r;
    outColor = vec4(oldHeight, 0.0, 0.0, 1.0);
}