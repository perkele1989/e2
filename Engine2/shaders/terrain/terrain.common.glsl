
#include <shaders/common/renderersets.glsl>

// Begin Set2: Material
layout(set = 2, binding = 0) uniform MaterialData
{
    vec4 albedo;
} material;

layout(set = 2, binding = 1) uniform texture2D mountainAlbedo;
layout(set = 2, binding = 2) uniform texture2D mountainNormal;
layout(set = 2, binding = 3) uniform texture2D sandAlbedo;
layout(set = 2, binding = 4) uniform texture2D sandNormal;
layout(set = 2, binding = 5) uniform texture2D greenAlbedo;
layout(set = 2, binding = 6) uniform texture2D greenNormal;
// End Set2
