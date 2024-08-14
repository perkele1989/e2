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
out vec3 fragmentNormal;
out vec4 fragmentTangent;
out vec4 fragmentColor;
#endif

#include <shaders/terrain/terrain.common.glsl>

void main()
{

	#if defined(Renderer_Shadow)
		vec4 fragmentColor;
	#endif

	vec4 worldPosition = mesh.modelMatrix * vertexPosition;

#if defined(Vertex_Color) 
	fragmentColor = vertexColor;
	//float tundraCoeff = fragmentColor.b;
	//worldPosition.y -= tundraCoeff * 0.05f;
#endif


#if defined(Renderer_Shadow)
	gl_Position = shadowViewProjection * worldPosition;
#else 
	gl_Position = renderer.projectionMatrix * renderer.viewMatrix * worldPosition;	
#endif

#if !defined(Renderer_Shadow)
	fragmentPosition = worldPosition;
	
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


#endif
}