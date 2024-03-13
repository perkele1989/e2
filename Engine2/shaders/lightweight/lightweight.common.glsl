#define EPSILON 1e-5

#include <shaders/common/renderersets.glsl>

// Begin Set2: Material
layout(set = 2, binding = 0) uniform MaterialData
{
    vec4 albedo; // albedo.rgb, reserved
    vec4 rmxx; // roughness, metalness, reserved, reserved
    vec4 emissive; // emissive.rgb, emissiveStrength.a
} material;

layout(set = 2, binding = 1) uniform texture2D albedoTexture;
layout(set = 2, binding = 2) uniform texture2D normalTexture;
layout(set = 2, binding = 3) uniform texture2D roughnessTexture;
layout(set = 2, binding = 4) uniform texture2D metalnessTexture;

//layout(set = 2, binding = 12) uniform texture2D visibilityMask;


// End Set2
