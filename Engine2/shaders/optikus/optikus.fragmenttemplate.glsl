#version 460 core

// Fragment attributes

#if defined(Use_Fragment_Position)
in vec4 fragmentPosition;
#endif

#if defined(Vertex_Normal) && defined(Use_Fragment_Normal)
in vec4 fragmentNormal;
in vec4 fragmentTangent;
in vec4 fragmentBitangent;
#endif

#if defined(Vertex_TexCoords01) && defined(Use_Fragment_TexCoords01)
in vec4 fragmentUv01;
#endif

#if defined(Vertex_TexCoords23) && defined(Use_Fragment_TexCoords23)
in vec4 fragmentUv23;
#endif

#if defined(Vertex_Color) && defined(Use_Fragment_Color)
in vec4 fragmentColor;
#endif

// Out color
out vec4 outColor;

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
/** endif */

/** foreach(texture : shadergraph.params) */
layout(set = 2, binding = 2) uniform texture2D albedoTexture;
/** endfor */

// End Set2

void main()
{
#if defined(Vertex_Normal) && defined(Use_Fragment_Normal)
    vec3 normal = normalize(fragmentNormal);
    vec3 tangent = normalize(fragmentTangent);
    vec3 bitangent = normalize(fragmentBitangent);
#endif

    /** main code goes here */
	outColor = vec4(1.0, 1.0, 1.0, 1.0);
}