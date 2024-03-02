
#version 450 core 

#include <shaders/lw/vertexattributes.glsl>

#include <shaders/entityuniforms.glsl>
#include <shaders/worlduniforms.glsl>

#include <shaders/lw/uniforms.glsl>

void main()
{
    mat4 mvMatrix = viewMatrix * modelMatrix;
    mat4 mvpMatrix = projectionMatrix * mvMatrix;
    mat4 nMatrix = transpose(inverse(mvMatrix));

    applyVertexAttributes(mvpMatrix, mvMatrix, nMatrix);

}
