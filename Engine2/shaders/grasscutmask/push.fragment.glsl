
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
    vec2 ndc = fragmentUv * 2.0 - 1.0;
    vec2 direction = ndc * radius;
    
    outColor.rgb = vec3(0.0, direction.x, direction.y);
    outColor.a = 1.0 - smoothstep(0.50, 0.95, length(ndc));
}