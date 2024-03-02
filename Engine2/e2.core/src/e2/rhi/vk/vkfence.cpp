
#include "e2/rhi/vk/vkfence.hpp"
#include "e2/rhi/vk/vkrendercontext.hpp"

e2::IFence_Vk::IFence_Vk(IRenderContext* context, e2::FenceCreateInfo const& createInfo)
	: e2::IFence(context, createInfo)
	, e2::ContextHolder_Vk(context)
{
	VkFenceCreateInfo vkCreateInfo{ VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };
	if (createInfo.createSignaled)
		vkCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

	VkResult result = vkCreateFence(m_renderContextVk->m_vkDevice, &vkCreateInfo, nullptr, &m_vkHandle);
	if (result != VK_SUCCESS)
	{
		LogError("vkCreateFence failed: {}", int32_t(result));
	}
}

e2::IFence_Vk::~IFence_Vk()
{
	vkDestroyFence(m_renderContextVk->m_vkDevice, m_vkHandle, nullptr);
}

bool e2::IFence_Vk::wait(uint64_t nsTime)
{
	VkResult result = vkWaitForFences(m_renderContextVk->m_vkDevice, 1, &m_vkHandle, VK_TRUE, nsTime);

	if (result == VK_SUCCESS)
	{
		return true;
	}
	else if(result == VK_TIMEOUT)
	{
		return false;
	}
	else
	{
		LogError("vkWaitForFences failed: {}", int32_t(result));
		return false;
	}
}

void e2::IFence_Vk::reset()
{
	vkResetFences(m_renderContextVk->m_vkDevice, 1, &m_vkHandle);
}
