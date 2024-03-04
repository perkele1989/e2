

#include <shaders/common/procgen.glsl>

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

layout(set = 2, binding = 1) uniform texture2D albedoTexture;
layout(set = 2, binding = 2) uniform sampler albedoSampler;
// End Set2

const mat2 myt = mat2(.12121212, .13131313, -.13131313, .12121212);
const vec2 mys = vec2(1e4, 1e6);

vec2 rhash(vec2 uv) {
    uv *= myt;
    uv *= mys;
    return fract(fract(uv / mys) * uv);
}

vec3 hash(vec3 p) {
    return fract(sin(vec3(dot(p, vec3(1.0, 57.0, 113.0)),
                        dot(p, vec3(57.0, 113.0, 1.0)),
                        dot(p, vec3(113.0, 1.0, 57.0)))) *
                43758.5453);
}

float voronoi2d(const in vec2 point) {
    vec2 p = floor(point);
    vec2 f = fract(point);
    float res = 0.0;
    for (int j = -1; j <= 1; j++) {
    for (int i = -1; i <= 1; i++) {
        vec2 b = vec2(i, j);
        vec2 r = vec2(b) - f + rhash(p + b);
        res += 1. / pow(dot(r, r), 8.);
    }
    }
    return pow(1. / res, 0.0625);
}


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

float sampleWaterHeight(vec2 position)
{
    //float depthCoeff = sampleWaterDepth(position);

    float variance1 = 0.2;
    float varianceOffset1 = 1.0 - variance1;
    float varianceSpeed1 = 3.0;
    float waveCoeff1 = (sin(renderer.time.x * varianceSpeed1) * 0.5 + 0.5) * variance1 + varianceOffset1;

    float variance2 = 0.5;
    float varianceOffset2 = 1.0 - variance2;
    float varianceSpeed2 = 2.5;
    float waveCoeff2 = (sin(renderer.time.x * varianceSpeed2) * 0.5 + 0.5) * variance2 + varianceOffset2;

    float speed1 = 0.5;
    vec2 dir1 = normalize(vec2(0.2, 0.4));
    vec2 pos1 = position + (dir1 * speed1 * renderer.time.x);
    float height1 = sampleHeight(pos1 * 2.0) * waveCoeff1;

    float speed2 = 0.3;
    vec2 dir2 = normalize(vec2(-0.22, -0.3));
    vec2 pos2 = position + (dir2 * speed2 * renderer.time.x);
    float height2 = sampleHeight(pos2) *  waveCoeff2;

    float weight1 = 0.36;
    float weight2 = (1.0 - weight1);//* depthCoeff;

    //float multiplier = mix(1.0, 1.0, depthCoeff);

    return (height1 * weight1 + height2 * weight2);// * multiplier;
}

vec3 sampleWaterNormal(vec2 position)
{
    float eps = 0.1;
    float eps2 = eps * 2;
    vec3 off = vec3(1.0, 1.0, 0.0)* eps;
    float hL = sampleWaterHeight(position.xy - off.xz);
    float hR = sampleWaterHeight(position.xy + off.xz);
    float hD = sampleWaterHeight(position.xy - off.zy);
    float hU = sampleWaterHeight(position.xy + off.zy);

    return normalize(vec3(hR - hL, -eps2 * 0.2, hU - hD));
}