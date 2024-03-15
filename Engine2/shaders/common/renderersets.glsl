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

layout(set = 0, binding = 1) uniform sampler clampSampler;
layout(set = 0, binding = 2) uniform sampler repeatSampler;
layout(set = 0, binding = 3) uniform sampler equirectSampler;
layout(set = 0, binding = 4) uniform texture2D integratedBrdf;

layout(set = 0, binding = 5) uniform texture2D irradianceCube;
layout(set = 0, binding = 6) uniform texture2D radianceCube;

layout(set = 0, binding = 7) uniform texture2D frontBufferColor;
layout(set = 0, binding = 8) uniform texture2D frontBufferPosition;
layout(set = 0, binding = 9) uniform texture2D frontBufferDepth;
// End Set0

// Begin Set1: Mesh 
layout(set = 1, binding = 0) uniform MeshData 
{
    mat4 modelMatrix;
} mesh;

layout(set = 1, binding = 1) uniform SkinData 
{
    mat4 skinMatrices[64]; // @todo make define
} skin;
// End Set1