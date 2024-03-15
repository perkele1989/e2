#version 460 core

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

// Fragment attributes

out vec4 fragmentPosition;

#if defined(Vertex_TexCoords01)
out vec4 fragmentUv01;
#endif

#if defined(Vertex_TexCoords23)
out vec4 fragmentUv23;
#endif 

#if defined(Vertex_Normals)
out vec3 fragmentNormal;
out vec4 fragmentTangent;
#endif

#if defined(Vertex_Color)
out vec4 fragmentColor;
#endif

#include <shaders/lightweight/lightweight.common.glsl>

void main()
{
	vec4 animatedVertexPosition = vertexPosition;

#if defined(Renderer_Skin) && defined(Vertex_Bones)

	mat4 animationTransform = skin.skinMatrices[vertexIds.x] * vertexWeights.x; 
	animationTransform += skin.skinMatrices[vertexIds.y] * vertexWeights.y; 
	animationTransform += skin.skinMatrices[vertexIds.z] * vertexWeights.z; 
	animationTransform += skin.skinMatrices[vertexIds.w] * vertexWeights.w; 

	animatedVertexPosition =  animationTransform * animatedVertexPosition;
#endif 

	gl_Position = renderer.projectionMatrix * renderer.viewMatrix * mesh.modelMatrix * animatedVertexPosition;

	fragmentPosition = mesh.modelMatrix * animatedVertexPosition;
#if defined(Vertex_Normals)

	fragmentNormal = normalize(mesh.modelMatrix * normalize(vertexNormal)).xyz;
	fragmentTangent.xyz =  normalize(mesh.modelMatrix * normalize(vec4(vertexTangent.xyz, 0.0))).xyz;
    fragmentTangent.w = vertexTangent.w;
	//fragmentBitangent = normalize(cross(fragmentNormal.xyz, fragmentTangent.xyz));
#endif

#if defined(Vertex_TexCoords01)
	fragmentUv01 = vertexUv01;
#endif

#if defined(Vertex_TexCoords23)
	fragmentUv23 = vertexUv23;
#endif

#if defined(Vertex_Color)
	fragmentColor = vertexColor;
#endif
}