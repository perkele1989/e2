
#include <shaders/common/renderersets.glsl>

// Begin Set2: Material
#if !defined(Renderer_Shadow)
layout(set = MaterialSetIndex, binding = 0) uniform MaterialData
{
    vec4 albedo; // albedo.rgb, reserved
    vec4 rmxx; // roughness, metalness, reserved, reserved
    vec4 emissive; // emissive.rgb, emissiveStrength.a
} material;

layout(set = MaterialSetIndex, binding = 1) uniform texture2D albedoTexture;
layout(set = MaterialSetIndex, binding = 2) uniform texture2D normalTexture;
layout(set = MaterialSetIndex, binding = 3) uniform texture2D roughnessTexture;
layout(set = MaterialSetIndex, binding = 4) uniform texture2D metalnessTexture;
#endif

//layout(set = 2, binding = 12) uniform texture2D visibilityMask;


// End Set2
