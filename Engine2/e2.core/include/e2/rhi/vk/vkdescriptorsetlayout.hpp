
#pragma once 

#include <e2/buildcfg.hpp>

#include <e2/rhi/descriptorsetlayout.hpp>
#include <e2/rhi/vk/vkresource.hpp>

#include <Volk/volk.h>

namespace e2
{
	/** @tags(arena, arenaSize=e2::maxVkDescriptorSetLayouts) */
	class E2_API IDescriptorSetLayout_Vk : public e2::IDescriptorSetLayout, public e2::ContextHolder_Vk
	{
		ObjectDeclaration()
	public:
		IDescriptorSetLayout_Vk(IRenderContext* context, e2::DescriptorSetLayoutCreateInfo const& createInfo);
		virtual ~IDescriptorSetLayout_Vk();

		VkDescriptorSetLayout m_vkHandle{};

	};
}

#include "vkdescriptorsetlayout.generated.hpp"