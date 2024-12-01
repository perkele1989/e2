
#version 460 core

in vec2 fragmentUv;
out vec4 outColor;

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
    vec4 previousMap = texture(sampler2D(areaTexture, areaSampler), fragmentUv).rgba;
    float cutCoefficient = previousMap.r;
    vec2 offset = previousMap.gb;

    if(length(offset) < 15.0)
        offset += vec2(15.0, 15.0) * timeDelta * 0.05;

    outColor = vec4(cutCoefficient, offset.x, offset.y, 1.0);
}