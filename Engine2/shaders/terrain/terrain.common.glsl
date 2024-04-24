
#include <shaders/common/renderersets.glsl>

#if !defined(Renderer_Shadow)
// Begin Set2: Material
layout(set = MaterialSetIndex, binding = 0) uniform MaterialData
{
    vec4 albedo;
} material;

layout(set = MaterialSetIndex, binding = 1) uniform texture2D mountainAlbedo;
layout(set = MaterialSetIndex, binding = 2) uniform texture2D mountainNormal;
layout(set = MaterialSetIndex, binding = 3) uniform texture2D sandAlbedo;
layout(set = MaterialSetIndex, binding = 4) uniform texture2D sandNormal;
layout(set = MaterialSetIndex, binding = 5) uniform texture2D greenAlbedo;
layout(set = MaterialSetIndex, binding = 6) uniform texture2D greenNormal;
//layout(set = MaterialSetIndex, binding = 7) uniform texture2D outlineTexture;
// End Set2
#endif