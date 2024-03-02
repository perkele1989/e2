
#include "e2/rhi/vk/vkvertexlayout.hpp"


namespace
{
	VkFormat e2ToVk(e2::VertexFormat vertexFormat)
	{
		switch (vertexFormat)
		{
		case e2::VertexFormat::Float:
			return VK_FORMAT_R32_SFLOAT;
			break;
		case e2::VertexFormat::Vec2:
			return VK_FORMAT_R32G32_SFLOAT;
			break;
		case e2::VertexFormat::Vec3:
			return VK_FORMAT_R32G32B32_SFLOAT;
			break;
		default:
		case e2::VertexFormat::Vec4:
			return VK_FORMAT_R32G32B32A32_SFLOAT;
			break;
		case e2::VertexFormat::Int:
			return VK_FORMAT_R32_SINT;
			break;
		case e2::VertexFormat::Vec2i:
			return VK_FORMAT_R32G32_SINT;
			break;
		case e2::VertexFormat::Vec3i:
			return VK_FORMAT_R32G32B32_SINT;
			break;
		case e2::VertexFormat::Vec4i:
			return VK_FORMAT_R32G32B32A32_SINT;
			break;
		case e2::VertexFormat::Uint:
			return VK_FORMAT_R32_UINT;
			break;
		case e2::VertexFormat::Vec2u:
			return VK_FORMAT_R32G32_UINT;
			break;
		case e2::VertexFormat::Vec3u:
			return VK_FORMAT_R32G32B32_UINT;
			break;
		case e2::VertexFormat::Vec4u:
			return VK_FORMAT_R32G32B32A32_UINT;
			break;
		}
	}

}

e2::IVertexLayout_Vk::IVertexLayout_Vk(IRenderContext* context, e2::VertexLayoutCreateInfo const& createInfo)
	: e2::IVertexLayout(context, createInfo)
	, e2::ContextHolder_Vk(context)
{
	uint32_t counter = 0;
	for (e2::VertexLayoutBinding binding : createInfo.bindings)
	{
		VkVertexInputBindingDescription2EXT newBinding{ VK_STRUCTURE_TYPE_VERTEX_INPUT_BINDING_DESCRIPTION_2_EXT };
		newBinding.divisor = 1;
		newBinding.stride = binding.stride;
		newBinding.inputRate = binding.rate == VertexRate::PerVertex ? VK_VERTEX_INPUT_RATE_VERTEX : VK_VERTEX_INPUT_RATE_INSTANCE;
		newBinding.binding = counter++;
		m_vkBindings.push(newBinding);
	}

	counter = 0;
	for (e2::VertexLayoutAttribute attribute : createInfo.attributes)
	{
		VkVertexInputAttributeDescription2EXT newAttribute{ VK_STRUCTURE_TYPE_VERTEX_INPUT_ATTRIBUTE_DESCRIPTION_2_EXT };
		newAttribute.binding = attribute.bindingIndex;
		newAttribute.format = ::e2ToVk(attribute.format);
		newAttribute.offset = attribute.offset;
		newAttribute.location = counter++;
		m_vkAttributes.push(newAttribute);
	}
}

e2::IVertexLayout_Vk::~IVertexLayout_Vk()
{

}
