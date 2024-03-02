
#pragma once 

#include <e2/buildcfg.hpp>

#include <e2/rhi/rendertarget.hpp>
#include <e2/rhi/vk/vkresource.hpp>

#include <Volk/volk.h>

namespace e2
{
	/** @tags(arena, arenaSize=e2::maxVkRenderTargets) */
	class E2_API IRenderTarget_Vk : public e2::IRenderTarget, public e2::ContextHolder_Vk
	{
		ObjectDeclaration()
	public:
		IRenderTarget_Vk(IRenderContext* context, e2::RenderTargetCreateInfo const& createInfo);
		virtual ~IRenderTarget_Vk();

		VkRenderingInfo m_vkRenderInfo{};
		e2::StackVector<VkRenderingAttachmentInfo, e2::maxNumRenderAttachments> m_colorAttachments;
		VkRenderingAttachmentInfo m_depthAttachment{ VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO };
		VkRenderingAttachmentInfo m_stencilAttachment{ VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO };
	};
}

#include "vkrendertarget.generated.hpp"