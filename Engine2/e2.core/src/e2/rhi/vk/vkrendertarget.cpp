
#include "e2/rhi/vk/vkrendertarget.hpp"
#include "e2/rhi/vk/vktexture.hpp"

namespace
{
	VkAttachmentLoadOp e2ToVk(e2::LoadOperation loadOp)
	{
		switch (loadOp)
		{
		case e2::LoadOperation::Ignore:
			return VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			break;
		case e2::LoadOperation::Clear:
			return VK_ATTACHMENT_LOAD_OP_CLEAR;
			break;
		default:
		case e2::LoadOperation::Load:
			return VK_ATTACHMENT_LOAD_OP_LOAD;
			break;
		}
	}

	VkAttachmentStoreOp e2ToVk(e2::StoreOperation storeOp)
	{
		switch (storeOp)
		{
		case e2::StoreOperation::DontCare:
			return VK_ATTACHMENT_STORE_OP_DONT_CARE;
			break;
		case e2::StoreOperation::None:
			return VK_ATTACHMENT_STORE_OP_NONE;
			break;
		default:
		case e2::StoreOperation::Store:
			return VK_ATTACHMENT_STORE_OP_STORE;
			break;
		}
	}

	void applyClearValue(VkClearValue& outClearValue, e2::ClearValue const& inClearValue, e2::ClearMethod inClearMethod)
	{
		if (inClearMethod == e2::ClearMethod::ColorFloat)
		{
			outClearValue.color.float32[0] = inClearValue.clearColorf32.x;
			outClearValue.color.float32[1] = inClearValue.clearColorf32.y;
			outClearValue.color.float32[2] = inClearValue.clearColorf32.z;
			outClearValue.color.float32[3] = inClearValue.clearColorf32.w;
		}
		else if (inClearMethod == e2::ClearMethod::ColorUint)
		{
			outClearValue.color.uint32[0] = inClearValue.clearColoru32.x;
			outClearValue.color.uint32[1] = inClearValue.clearColoru32.y;
			outClearValue.color.uint32[2] = inClearValue.clearColoru32.z;
			outClearValue.color.uint32[3] = inClearValue.clearColoru32.w;
		}
		else if (inClearMethod == e2::ClearMethod::ColorInt)
		{
			outClearValue.color.int32[0] = inClearValue.clearColori32.x;
			outClearValue.color.int32[1] = inClearValue.clearColori32.y;
			outClearValue.color.int32[2] = inClearValue.clearColori32.z;
			outClearValue.color.int32[3] = inClearValue.clearColori32.w;
		}
		else if (inClearMethod == e2::ClearMethod::DepthStencil)
		{
			outClearValue.depthStencil.depth = inClearValue.depth;
			outClearValue.depthStencil.stencil = inClearValue.stencil;
		}
	}
}

e2::IRenderTarget_Vk::IRenderTarget_Vk(IRenderContext* context, e2::RenderTargetCreateInfo const& createInfo)
	: e2::IRenderTarget(context, createInfo)
	, e2::ContextHolder_Vk(context)
{
	m_vkRenderInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
	m_vkRenderInfo.pNext = nullptr;
	m_vkRenderInfo.flags = 0;
	m_vkRenderInfo.renderArea.extent.width = createInfo.areaExtent.x;
	m_vkRenderInfo.renderArea.extent.height = createInfo.areaExtent.y;
	m_vkRenderInfo.renderArea.offset.x = createInfo.areaOffset.x;
	m_vkRenderInfo.renderArea.offset.y = createInfo.areaOffset.y;
	m_vkRenderInfo.layerCount = createInfo.layerCount;
	m_vkRenderInfo.viewMask = 0;
	m_vkRenderInfo.colorAttachmentCount = (uint32_t)createInfo.colorAttachments.size();

	for (e2::RenderAttachment const& attachment : createInfo.colorAttachments)
	{
		e2::ITexture_Vk* vkTexture = static_cast<e2::ITexture_Vk*>(attachment.target);

		VkRenderingAttachmentInfo colorInfo{ VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO };
		colorInfo.imageView = vkTexture->m_vkImageView;
		colorInfo.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		colorInfo.resolveMode = VK_RESOLVE_MODE_NONE;
		colorInfo.loadOp = ::e2ToVk(attachment.loadOperation);
		colorInfo.storeOp = ::e2ToVk(attachment.storeOperation);
		::applyClearValue(colorInfo.clearValue, attachment.clearValue, attachment.clearMethod);
		m_colorAttachments.push(colorInfo);
	}

	m_vkRenderInfo.pColorAttachments = m_colorAttachments.data();

	VkImageLayout dsLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL;
	if (createInfo.depthAttachment.target && createInfo.stencilAttachment.target)
	{
		dsLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
	}
	else if (createInfo.stencilAttachment.target)
	{
		dsLayout = VK_IMAGE_LAYOUT_STENCIL_ATTACHMENT_OPTIMAL;
	}
	else
	{
		dsLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
	}

	if (createInfo.depthAttachment.target)
	{
		e2::ITexture_Vk* vkTexture = static_cast<e2::ITexture_Vk*>(createInfo.depthAttachment.target);

		m_depthAttachment.imageView = vkTexture->m_vkImageView;
		m_depthAttachment.imageLayout = dsLayout;
		m_depthAttachment.resolveMode = VK_RESOLVE_MODE_NONE;
		m_depthAttachment.loadOp = ::e2ToVk(createInfo.depthAttachment.loadOperation);
		m_depthAttachment.storeOp = ::e2ToVk(createInfo.depthAttachment.storeOperation);
		::applyClearValue(m_depthAttachment.clearValue, createInfo.depthAttachment.clearValue, createInfo.depthAttachment.clearMethod);

		m_vkRenderInfo.pDepthAttachment = &m_depthAttachment;
	}

	if (createInfo.stencilAttachment.target)
	{
		e2::ITexture_Vk* vkTexture = static_cast<e2::ITexture_Vk*>(createInfo.stencilAttachment.target);

		m_stencilAttachment.imageView = vkTexture->m_vkImageView;
		m_stencilAttachment.imageLayout = dsLayout;
		m_stencilAttachment.resolveMode = VK_RESOLVE_MODE_NONE;
		m_stencilAttachment.loadOp = ::e2ToVk(createInfo.stencilAttachment.loadOperation);
		m_stencilAttachment.storeOp = ::e2ToVk(createInfo.stencilAttachment.storeOperation);
		::applyClearValue(m_stencilAttachment.clearValue, createInfo.stencilAttachment.clearValue, createInfo.stencilAttachment.clearMethod);

		m_vkRenderInfo.pStencilAttachment = &m_stencilAttachment;
	}
}

e2::IRenderTarget_Vk::~IRenderTarget_Vk()
{

}
