

#include <shaders/common/utils.glsl>

// Push constants
layout(push_constant) uniform ConstantData
{
    mat4 normalMatrix;
    uvec2 resolution;
};

// Begin Set0: Renderer
layout(set = 0, binding = 0) uniform RendererData
{
    mat4 viewMatrix;
    mat4 projectionMatrix;
    vec4 time; // t, sin(t), cos(t), tan(t)
} renderer;

layout(set = 0, binding = 1)  uniform texture2D integratedBrdf;
layout(set = 0, binding = 2) uniform sampler brdfSampler;

layout(set = 0, binding = 3)  uniform texture2D frontBufferColor;
layout(set = 0, binding = 4)  uniform texture2D frontBufferPosition;
layout(set = 0, binding = 5)  uniform texture2D frontBufferDepth;
layout(set = 0, binding = 6) uniform sampler frontBufferSampler;
// End Set0

// Begin Set1: Mesh 
layout(set = 1, binding = 0) uniform MeshData 
{
    mat4 modelMatrix;
} mesh;
// End Set1

// Begin Set2: Material
layout(set = 2, binding = 0) uniform MaterialData
{
    vec4 albedo;
} material;

layout(set = 2, binding = 1) uniform texture2D reflectionHdr;
layout(set = 2, binding = 2) uniform sampler genericSampler;
layout(set = 2, binding = 3) uniform texture2D visibilityMask;
// End Set2



float sampleHeight(vec2 position)
{
    return pow(voronoi2d(position), 2.0);
}

vec3 sampleNormal(vec2 position)
{
    float eps = 0.1;
    float eps2 = eps * 2;
    vec3 off = vec3(1.0, 1.0, 0.0)* eps;
    float hL = sampleHeight(position.xy - off.xz);
    float hR = sampleHeight(position.xy + off.xz);
    float hD = sampleHeight(position.xy - off.zy);
    float hU = sampleHeight(position.xy + off.zy);

    return normalize(vec3(hL - hR, -eps2, hD - hU));
}

float sampleWaterDepth(vec2 position)
{
    float baseHeight = sampleBaseHeight(position);

    float depthCoeff = smoothstep(0.03, 0.39, baseHeight);

    return 1.0 - depthCoeff;
}

float sampleWaterHeight(vec2 position, float time)
{
    //float depthCoeff = sampleWaterDepth(position);

    float s = simplex(position * 0.2) * 0.5 + 0.5;
    float size = mix(1.3, 2.2, s);

    //return s;

    float variance1 = 0.2;
    float varianceOffset1 = 1.0 - variance1;
    float varianceSpeed1 = 3.0;
    float waveCoeff1 = (sin(time * varianceSpeed1) * 0.5 + 0.5) * variance1 + varianceOffset1;

    float variance2 = 0.5;
    float varianceOffset2 = 1.0 - variance2;
    float varianceSpeed2 = 2.5;
    float waveCoeff2 = (sin(time * varianceSpeed2) * 0.5 + 0.5) * variance2 + varianceOffset2;

    float speed1 = 0.5;
    vec2 dir1 = normalize(vec2(0.2, 0.4));
    vec2 pos1 = position + (dir1 * speed1 * time);
    float height1 = sampleHeight(pos1 * 2.0) * waveCoeff1;

    float speed2 = 0.3;
    vec2 dir2 = normalize(vec2(-0.22, -0.3));
    vec2 pos2 = position + (dir2 * speed2 * time);
    float height2 = sampleHeight(pos2) *  waveCoeff2;

    float weight1 = 0.36;
    float weight2 = (1.0 - weight1);//* depthCoeff;

    //float multiplier = mix(1.0, 1.0, depthCoeff);

    return (height1 * weight1 + height2 * weight2);// * multiplier;
}

vec3 sampleWaterNormal(vec2 position, float time)
{
    float eps = 0.1;
    float eps2 = eps * 2;
    vec3 off = vec3(1.0, 1.0, 0.0)* eps;
    float hL = sampleWaterHeight(position.xy - off.xz, time);
    float hR = sampleWaterHeight(position.xy + off.xz, time);
    float hD = sampleWaterHeight(position.xy - off.zy, time);
    float hU = sampleWaterHeight(position.xy + off.zy, time);

    return normalize(vec3(hR - hL, -eps2 * 0.2, hU - hD));
}