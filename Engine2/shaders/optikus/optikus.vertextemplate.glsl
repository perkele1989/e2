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

#if defined(Use_Fragment_Position)
out vec4 fragmentPosition;
#endif

#if defined(Vertex_Normal) && defined(Use_Fragment_Normal)
out vec4 fragmentNormal;
out vec4 fragmentTangent;
out vec4 fragmentBitangent;
#endif

#if defined(Vertex_TexCoords01) && defined(Use_Fragment_TexCoords01)
out vec4 fragmentUv01;
#endif

#if defined(Vertex_TexCoords23) && defined(Use_Fragment_TexCoords23)
out vec4 fragmentUv23;
#endif

#if defined(Vertex_Color) && defined(Use_Fragment_Color)
out vec4 fragmentColor;
#endif

// Push constants
layout(push_constant) uniform ConstantData
{
	mat4 normalMatrix;
};

// Begin Set0: Renderer
layout(set = 0, binding = 0) uniform RendererData
{
	mat4 viewMatrix;
	mat4 projectionMatrix;
	vec4 time; // t, sin(t), cos(t), tan(t)
} renderer;
// End Set0

// Begin Set1: Mesh 
layout(set = 1, binding = 0) uniform MeshData 
{
	mat4 modelMatrix;
} mesh;
// End Set1

// Begin Set2: Material
layout(set = 2, binding = 0) uniform MaterialData
{
	/** foreach(param : shadergraph.params) */
	vec4 albedo;
	/** endfor */
} material;

/** if (numTextures > 0) */
layout(set = 2, binding = 1) uniform sampler albedoSampler;

/** foreach(texture : shadergraph.params) */
layout(set = 2, binding = 2) uniform texture2D albedoTexture;
/** endfor */

// End Set2

void main()
{
	gl_Position = renderer.projectionMatrix * renderer.viewMatrix * mesh.modelMatrix * vertexPosition;

	// Write fragment attributes (in worldspace where applicable)
	// @todo skinned 
#if defined(Use_Fragment_Position)
	fragmentPosition = renderer.viewMatrix * mesh.modelMatrix * vertexPosition;
#endif

#if defined(Vertex_Normal) && defined(Use_Fragment_Normal)
	fragmentNormal = vec3(normalMatrix * vertexNormal);
	fragmentTangent = vec3(normalMatrix * vertexTangent);
	fragmentBitangent = cross(fragmentNormal, fragmentTangent);
#endif

#if defined(Vertex_TexCoords01) && defined(Use_Fragment_TexCoords01)
	fragmentUv01 = vertexUv01;
#endif

#if defined(Vertex_TexCoords23) && defined(Use_Fragment_TexCoords23)
	fragmentUv23 = vertexUv23;
#endif

#if defined(Vertex_Color) && defined(Use_Fragment_Color)
	fragmentColor = vertexColor;
#endif
}