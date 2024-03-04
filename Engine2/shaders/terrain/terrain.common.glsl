#define EPSILON 1e-5

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

layout(set = 2, binding = 1) uniform sampler mainSampler;
layout(set = 2, binding = 2) uniform texture2D irradianceCube;
layout(set = 2, binding = 3) uniform texture2D radianceCube;

layout(set = 2, binding = 4) uniform texture2D mountainAlbedo;
layout(set = 2, binding = 5) uniform texture2D mountainNormal;

layout(set = 2, binding = 6) uniform texture2D fieldsAlbedo;
layout(set = 2, binding = 7) uniform texture2D fieldsNormal;

layout(set = 2, binding = 8) uniform texture2D sandAlbedo;
layout(set = 2, binding = 9) uniform texture2D sandNormal;

layout(set = 2, binding = 10) uniform texture2D greenAlbedo;
layout(set = 2, binding = 11) uniform texture2D greenNormal;


// End Set2
