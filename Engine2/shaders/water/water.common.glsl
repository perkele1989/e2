

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


float generateWaveHeight(
    vec2 position,
    float time,
    float scale1,
    float scale2,
    float scale3,
    float variance,
    float varianceSpeed,
    float speed,
    vec2 dir)
{
    float varianceOffset = 1.0 - variance;
    float waveCoeff = (sin(time * varianceSpeed) * 0.5 + 0.5) * variance + varianceOffset;

    dir = normalize(dir);
    vec2 pos = position + (dir * speed * time);
    return sampleHeight(pos*scale3 + scale1 * scale2) * waveCoeff;
}

float sampleWaterHeight(vec2 position, float time)
{
    // float height1 = generateWaveHeight(position, time,
    //  0.0, 1.6, 1.0,
    //  0.2, 1.0,
    //  0.2, vec2(0.2, 0.4));

    // float height2 = generateWaveHeight(position, time,
    // height1*0.314, 1.0, 1.0,
    // 0.5, 0.5,
    // 0.3, vec2(-0.22, -0.3));

    // float height3 = generateWaveHeight(position, time,
    // height1*0.3, 1.0, 3.0,
    // 0.5, 0.5,
    // 0.1, vec2(0.22, 0.4));


/// play area
    float heightSmallOffset = generateWaveHeight(position, time,
     0.0, 1.6, 3.5,
     0.2, 1.0,
     0.1, vec2(0.2, 0.4));

    float heightSmall = generateWaveHeight(position, time,
    heightSmallOffset*0.314, 1.0, 2.5,
    0.5, 0.5,
    0.15, vec2(-0.22, -0.3));




    float heightBigOffset = generateWaveHeight(position, time,
     0.0, 1.6, 1.5,
     0.2, 1.0,
     0.2, vec2(0.2, 0.4));

    float heightBig = generateWaveHeight(position, time,
    heightBigOffset*0.314, 1.0, 1.0,
    0.5, 0.5,
    0.3, vec2(-0.22, -0.3));


    return heightSmall * 0.2 + heightBig * 0.8;

    // vec3 weights = normalize(vec3(0.2, 0.8, 0.143));
    // return (height1 * weights.x + height2 * weights.y + height3 * weights.z);

}

const float heightScale = 0.15;

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