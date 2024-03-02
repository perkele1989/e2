
layout(std140) uniform material 
{
    vec4 albedo;
}

#if defined(LW_HasAlbedoTexture)
uniform sampler2D albedoTexture;
#endif
