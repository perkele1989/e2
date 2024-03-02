
#version 450 core 

#include <shaders/lw/fragmentattributes.glsl>

#include <shaders/entityuniforms.glsl>
#include <shaders/worlduniforms.glsl>

#include <shaders/lw/uniforms.glsl>

void main()
{
    #if defined(LW_HasAlbedoTexture) && defined(LW_HasUv0)
    output = texture(albedoTexture, fragmentUv0) * albedo;
    #else 
    output = albedo;
    #endif
}