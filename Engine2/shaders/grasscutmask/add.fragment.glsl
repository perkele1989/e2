



#version 460 core

in vec2 fragmentUv;
out vec4 outColor;

layout(push_constant) uniform ConstantData
{
    vec2 resolution;
    vec2 areaSize;
    vec2 areaCenter;

    vec2 position;
    float radius;
};

void main()
{
    outColor.rgb = vec3(0.0, 0.0, 0.0);
    outColor.a = 1.0 - step(0.45, distance(fragmentUv, vec2(0.5, 0.5)));
}