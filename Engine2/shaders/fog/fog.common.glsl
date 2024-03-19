


#include <shaders/common/renderersets.glsl>

// Begin Set2: Material
layout(set = 2, binding = 0) uniform MaterialData
{
    vec4 albedo;
} material;

layout(set = 2, binding = 1) uniform texture2D visibilityMask;
//layout(set = 2, binding = 3) uniform texture2D visibilityMask;
// End Set2
