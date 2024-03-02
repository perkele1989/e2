
#pragma once 

#include <e2/buildcfg.hpp>

#include <e2/rhi/pipeline.hpp>
#include <e2/rhi/vk/vkresource.hpp>

#include <Volk/volk.h>

namespace e2
{
	/** @tags(arena, arenaSize=e2::maxVkPipelineLayouts) */
	class E2_API IPipelineLayout_Vk : public e2::IPipelineLayout, public e2::ContextHolder_Vk
	{
		ObjectDeclaration()
	public:
		IPipelineLayout_Vk(e2::IRenderContext* context, e2::PipelineLayoutCreateInfo const& createInfo);
		virtual ~IPipelineLayout_Vk();

		VkPipelineLayout m_vkHandle{};
	};


	/** @tags(arena, arenaSize=e2::maxVkPipelines) */
	class E2_API IPipeline_Vk : public e2::IPipeline, public e2::ContextHolder_Vk
	{
		ObjectDeclaration()
	public:
		IPipeline_Vk(IRenderContext* context, e2::PipelineCreateInfo const& createInfo);
		virtual ~IPipeline_Vk();


		VkPipeline m_vkHandle{};
	};
}

#include "vkpipeline.generated.hpp"