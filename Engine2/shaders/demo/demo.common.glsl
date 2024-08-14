

#include <shaders/common/renderersets.glsl>

// Begin Set2: Material
#if !defined(Renderer_Shadow)
layout(set = MaterialSetIndex, binding = 0) uniform MaterialData
{
    vec4 albedo;
    vec4 water;
    uvec4 water2;
} material;

layout(set = MaterialSetIndex, binding = 1) uniform texture2D reflectionHdr;

#endif
// End Set2



float sampleHeight(vec2 position)
{
    return pow(voronoi2d(position * 2.0), 2.0);
}

float sampleWaterHeight(vec2 position, float time)
{
    if((material.water2.x == 1 || material.water2.x == 2) && material.water2.y == 0)
    {
        return sampleHeight(position);
    }

    float variance1 = 0.2;
    float varianceOffset1 = 1.0 - variance1;
    float varianceSpeed1 = 3.0;
    float waveCoeff1 = (sin(time * varianceSpeed1) * 0.5 + 0.5) * variance1 + varianceOffset1;

    float variance2 = 0.5;
    float varianceOffset2 = 1.0 - variance2;
    float varianceSpeed2 = 2.5;
    float waveCoeff2 = (sin(time * varianceSpeed2) * 0.5 + 0.5) * variance2 + varianceOffset2;

    float speed1 = 0.2;
    vec2 dir1 = normalize(vec2(0.2, 0.4));
    vec2 pos1 = position + (dir1 * speed1 * time);
    float height1 = sampleHeight(pos1 * 1.6) * waveCoeff1;

    float speed2 = 0.3;
    vec2 dir2 = normalize(vec2(-0.22, -0.3));
    vec2 pos2 = position + (dir2 * speed2 * time);
    float height2 = sampleHeight(pos2 + vec2(1.0, 1.0) * height1 * 0.3) *  waveCoeff2;

    float weight1 = 0.3;
    float weight2 = (1.0 - weight1);

    return (height1 * weight1 + height2 * weight2);
}

vec3 sampleWaterNormal(vec2 position, float time)
{
    float eps = material.water.z;
    float str = material.water.y;
    float eps2 = eps * 2;

    vec3 off = vec3(1.0, 1.0, 0.0)* eps;
    float hL = sampleWaterHeight(position.xy - off.xz, time) * material.water.w;
    float hR = sampleWaterHeight(position.xy + off.xz, time) * material.water.w;
    float hU = sampleWaterHeight(position.xy - off.zy, time) * material.water.w;
    float hD = sampleWaterHeight(position.xy + off.zy, time) * material.water.w;

    return -normalize(vec3(hR - hL, eps2 / str, hD - hU));
}