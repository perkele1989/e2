
#include "e2/rhi/vk/vksemaphore.hpp"
#include "e2/rhi/vk/vkrendercontext.hpp"

e2::ISemaphore_Vk::ISemaphore_Vk(IRenderContext* context, e2::SemaphoreCreateInfo const& createInfo)
	: e2::ISemaphore(context, createInfo)
	, e2::ContextHolder_Vk(context)
{
	VkSemaphoreCreateInfo vkCreateInfo{ VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };

	VkResult result = vkCreateSemaphore(m_renderContextVk->m_vkDevice, &vkCreateInfo, nullptr, &m_vkHandle);
	if (result != VK_SUCCESS)
	{
		LogError("vkCreateSemaphore failed: {}", int32_t(result));
	}
}

e2::ISemaphore_Vk::~ISemaphore_Vk()
{
	vkDestroySemaphore(m_renderContextVk->m_vkDevice, m_vkHandle, nullptr);
}
