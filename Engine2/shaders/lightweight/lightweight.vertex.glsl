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
	vec4 animatedVertexPosition = vec4(vertexPosition.xyz, 1.0);
	vec4 animatedVertexNormal = vec4(vertexNormal.xyz, 0.0);
	vec4 animatedVertexTangent = vec4(vertexTangent.xyz, 0.0);
	float tangentSign = vertexTangent.w;

#if defined(Renderer_Skin) && defined(Vertex_Bones)

	vec4 weights = normalize(vertexWeights);
	weights = vertexWeights;

	mat4 animationTransform = skin.skinMatrices[vertexIds.x] * weights.x; 
	animationTransform += skin.skinMatrices[vertexIds.y] * weights.y; 
	animationTransform += skin.skinMatrices[vertexIds.z] * weights.z; 
	animationTransform += skin.skinMatrices[vertexIds.w] * weights.w; 

	animatedVertexPosition =  animationTransform * animatedVertexPosition;
	animatedVertexNormal = animationTransform * animatedVertexNormal;
	animatedVertexTangent = animationTransform *animatedVertexTangent;

	animatedVertexPosition.xyz /= animatedVertexPosition.w;
	animatedVertexPosition.w = 1.0;
#endif 

	gl_Position = renderer.projectionMatrix * renderer.viewMatrix * mesh.modelMatrix * animatedVertexPosition;

	fragmentPosition = mesh.modelMatrix * animatedVertexPosition;
#if defined(Vertex_Normals)

	fragmentNormal = normalize(mesh.modelMatrix * normalize(animatedVertexNormal)).xyz;
	fragmentTangent.xyz =  normalize(mesh.modelMatrix * normalize(animatedVertexTangent)).xyz;
    fragmentTangent.w = tangentSign;
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