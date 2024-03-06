
#include "e2/renderer/meshspecification.hpp"


/*E2_API uint64_t e2::vertexFormatSize(e2::VertexFormat type)
{
	switch (type)
	{
	case e2::VertexFormat::Vec2:
		return sizeof(glm::vec2);
	case e2::VertexFormat::Vec3:
		return sizeof(glm::vec3);
	case e2::VertexFormat::Vec4:
		return sizeof(glm::vec4);
	case e2::VertexFormat::Vec2u:
		return sizeof(glm::uvec2);
	case e2::VertexFormat::Vec3u:
		return sizeof(glm::uvec3);
	case e2::VertexFormat::Vec4u:
		return sizeof(glm::uvec4);
	case e2::VertexFormat::Vec2i:
		return sizeof(glm::ivec2);
	case e2::VertexFormat::Vec3i:
		return sizeof(glm::ivec3);
	case e2::VertexFormat::Vec4i:
		return sizeof(glm::ivec4);
	default:
		return 0;
	}
}*/

void e2::applyVertexAttributeDefines(VertexAttributeFlags flags, ShaderCreateInfo& outInfo)
{
	// @todo utility functions to get defines from these 
	if ((flags & e2::VertexAttributeFlags::Normal) == e2::VertexAttributeFlags::Normal)
		outInfo.defines.push({ "Vertex_Normals", "1" });

	if ((flags & e2::VertexAttributeFlags::TexCoords01) == e2::VertexAttributeFlags::TexCoords01)
		outInfo.defines.push({ "Vertex_TexCoords01", "1" });

	if ((flags & e2::VertexAttributeFlags::TexCoords23) == e2::VertexAttributeFlags::TexCoords23)
		outInfo.defines.push({ "Vertex_TexCoords23", "1" });

	if ((flags & e2::VertexAttributeFlags::Color) == e2::VertexAttributeFlags::Color)
		outInfo.defines.push({ "Vertex_Color", "1" });

	if ((flags & e2::VertexAttributeFlags::Bones) == e2::VertexAttributeFlags::Bones)
		outInfo.defines.push({ "Vertex_Bones", "1" });

}
