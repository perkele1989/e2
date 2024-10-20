

#include <shaders/common/renderersets.glsl>

// Begin Set2: Material
#if !defined(Renderer_Shadow)
layout(set = MaterialSetIndex, binding = 0) uniform MaterialData
{
    vec4 albedo;
} material;

layout(set = MaterialSetIndex, binding = 1) uniform texture2D reflectionHdr;
layout(set = MaterialSetIndex, binding = 2) uniform texture2D visibilityMask;
#endif
// End Set2


float sampleHeight(vec2 position)
{
    return pow(voronoi2d(position*0.8), 2.0);
}

float sampleWaterHeight(vec2 position, float time)
{
    float variance1 = 0.2;
    float varianceOffset1 = 1.0 - variance1;
    float varianceSpeed1 = 1.0;
    float waveCoeff1 = (sin(time * varianceSpeed1) * 0.5 + 0.5) * variance1 + varianceOffset1;

    float variance2 = 0.5;
    float varianceOffset2 = 1.0 - variance2;
    float varianceSpeed2 = 0.5;
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

const float heightScale = 0.25;

vec3 sampleWaterNormal(vec2 position, float time)
{
    float eps = 0.015;
    float str = 40.0;
    float eps2 = eps * 2;

    vec3 off = vec3(1.0, 1.0, 0.0)* eps;
    float hL = sampleWaterHeight(position.xy - off.xz, time) * heightScale;
    float hR = sampleWaterHeight(position.xy + off.xz, time) * heightScale;
    float hU = sampleWaterHeight(position.xy - off.zy, time) * heightScale;
    float hD = sampleWaterHeight(position.xy + off.zy, time) * heightScale;

    return -normalize(vec3(hR - hL, eps2 / str, hD - hU));
}