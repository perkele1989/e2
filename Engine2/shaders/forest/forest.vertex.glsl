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

// Fragment attributes

#if !defined(Renderer_Shadow)
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
#endif

#include <shaders/common/renderersets.glsl>

%CustomDescriptorSets%

void main()
{
	vec4 animatedVertexPosition = vec4(vertexPosition.xyz, 1.0);

#if !defined(Renderer_Shadow)
	vec4 animatedVertexNormal = vec4(vertexNormal.xyz, 0.0);
	vec4 animatedVertexTangent = vec4(vertexTangent.xyz, 0.0);
	float tangentSign = vertexTangent.w;
#endif

#if defined(Renderer_Skin) && defined(Vertex_Bones)

	mat4 animationTransform = skin.skinMatrices[vertexIds.x] * vertexWeights.x; 
	animationTransform += skin.skinMatrices[vertexIds.y] * vertexWeights.y; 
	animationTransform += skin.skinMatrices[vertexIds.z] * vertexWeights.z; 
	animationTransform += skin.skinMatrices[vertexIds.w] * vertexWeights.w; 

	animatedVertexPosition =  animationTransform * animatedVertexPosition;

#if !defined(Renderer_Shadow)
	animatedVertexNormal = animationTransform * animatedVertexNormal;
	animatedVertexTangent = animationTransform *animatedVertexTangent;
#endif

	animatedVertexPosition.xyz /= animatedVertexPosition.w;
	animatedVertexPosition.w = 1.0;
#endif 


	vec4 modelPos = mesh.modelMatrix * animatedVertexPosition;

#if !defined(Renderer_Shadow)

	vec4 mmp = mesh.modelMatrix * vec4(0.0, 0.0, 0.0, 1.0);
	float ss = sampleSimplex(mmp.xz * 0.5) * 0.5 + 0.5;

	modelPos.x += cos(renderer.time.x*1.3 + ss*20.0) * mix(0.065 * ss * 0.75, 0.0, smoothstep(-0.7, 0.0, modelPos.y));
	modelPos.z += sin(renderer.time.x*0.7 + ss*40.0) * mix(0.073 * ss * 0.75, 0.0, smoothstep(-0.7, 0.0, modelPos.y));

	gl_Position = renderer.projectionMatrix * renderer.viewMatrix * modelPos;
#else 
	gl_Position = shadowViewProjection * mesh.modelMatrix * animatedVertexPosition;
#endif

#if !defined(Renderer_Shadow)
	fragmentPosition = modelPos;
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
#endif
}