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

out vec4 fragmentPosition;

#include <shaders/sky/sky.common.glsl>

void main()
{
    
	gl_Position = renderer.projectionMatrix * renderer.viewMatrix * mesh.modelMatrix * vertexPosition;

	fragmentPosition = mesh.modelMatrix * vertexPosition;
    
#if defined(Vertex_Normals)
	fragmentNormal = normalize(mesh.modelMatrix * normalize(vertexNormal)).xyz;
	fragmentTangent =  normalize(mesh.modelMatrix * normalize(vertexTangent)).xyz;
	fragmentBitangent = normalize(cross(fragmentNormal.xyz, fragmentTangent.xyz));
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