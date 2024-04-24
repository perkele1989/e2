#include <shaders/header.glsl>

// Vertex attributes 
in vec4 vertexPosition;

#if defined(Vertex_Normals)
in vec4 vertexNormal;
in vec4 vertexTangent;
#endif

#if defined(Vertex_TexCoords01)
in vec4 vertexUv01;
#endif 

#if defined(Vertex_TexCoords23)
in vec4 vertexUv23;
#endif 

#if defined(Vertex_Color)
in vec4 vertexColor;
#endif

#if defined(Vertex_Bones)
in vec4 vertexWeights;
in uvec4 vertexIds;
#endif

// Push constants
layout(push_constant) uniform ConstantData
{
    mat4 mvpMatrix;
	vec3 visibility;
};

void main()
{
	gl_Position = mvpMatrix * vertexPosition;
}