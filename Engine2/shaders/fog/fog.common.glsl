


#include <shaders/common/renderersets.glsl>

// Begin Set2: Material
layout(set = MaterialSetIndex, binding = 0) uniform MaterialData
{
    vec4 albedo;
} material;

layout(set = MaterialSetIndex, binding = 1) uniform texture2D visibilityMask;
//layout(set = MaterialSetIndex, binding = 3) uniform texture2D visibilityMask;
// End Set2
