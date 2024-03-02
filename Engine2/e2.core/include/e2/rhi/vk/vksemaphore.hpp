
#pragma once 

#include <e2/buildcfg.hpp>

#include <e2/rhi/semaphore.hpp>
#include <e2/rhi/vk/vkresource.hpp>

#include <Volk/volk.h>

namespace e2
{
	/** @tags(arena, arenaSize=e2::maxVkSemaphores) */
	class E2_API ISemaphore_Vk : public e2::ISemaphore, public e2::ContextHolder_Vk
	{
		ObjectDeclaration()
	public:
		ISemaphore_Vk(IRenderContext* context, e2::SemaphoreCreateInfo const& createInfo);
		virtual ~ISemaphore_Vk();

		VkSemaphore m_vkHandle{};
	};
}

#include "vksemaphore.generated.hpp"