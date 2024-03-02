
#pragma once 

#include <e2/buildcfg.hpp>

#include <e2/rhi/fence.hpp>
#include <e2/rhi/vk/vkresource.hpp>

#include <Volk/volk.h>

namespace e2
{
	/** @tags(arena, arenaSize=e2::maxVkFences) */
	class E2_API IFence_Vk : public e2::IFence, public e2::ContextHolder_Vk
	{
		ObjectDeclaration()
	public:
		IFence_Vk(IRenderContext* context, e2::FenceCreateInfo const& createInfo);
		virtual ~IFence_Vk();

		virtual bool wait(uint64_t nsTime = UINT64_MAX) override;
		virtual void reset() override;

		VkFence m_vkHandle{};
	};
}

#include "vkfence.generated.hpp"