
#include "e2/rhi/vk/vkdescriptorsetlayout.hpp"
#include "e2/rhi/vk/vkrendercontext.hpp"

e2::IDescriptorSetLayout_Vk::IDescriptorSetLayout_Vk(IRenderContext* context, e2::DescriptorSetLayoutCreateInfo const& createInfo)
	: IDescriptorSetLayout(context, createInfo)
	, ContextHolder_Vk(context)
{

	e2::StackVector<VkDescriptorSetLayoutBinding, e2::maxSetBindings> vkBindings;
	e2::StackVector<VkDescriptorBindingFlags, e2::maxSetBindings> vkBindingFlags;
	vkBindings.resize(createInfo.bindings.size());
	vkBindingFlags.resize(createInfo.bindings.size());

	bool shouldUpdateAfterBind = false;

	uint32_t i = 0;
	for (e2::DescriptorSetBinding const& binding : createInfo.bindings)
	{
		VkDescriptorBindingFlags& flags = vkBindingFlags[i];
		flags = 0;
		if (binding.allowPartiallyBound)
			flags |= VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT;

		if (binding.updateAfterBind)
		{
			flags |= VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT;
			shouldUpdateAfterBind = true;
		}


		VkDescriptorSetLayoutBinding &newBinding = vkBindings[i];
		newBinding.pImmutableSamplers = nullptr;
		newBinding.stageFlags = VK_SHADER_STAGE_ALL;
		newBinding.binding = i++;
		newBinding.descriptorCount = binding.count;
		newBinding.descriptorType =
			binding.type == DescriptorBindingType::Sampler ? VK_DESCRIPTOR_TYPE_SAMPLER :
			binding.type == DescriptorBindingType::Texture ? VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE :
			binding.type == DescriptorBindingType::UniformBuffer ? VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER :
			binding.type == DescriptorBindingType::StorageBuffer ? VK_DESCRIPTOR_TYPE_STORAGE_BUFFER :
			binding.type == DescriptorBindingType::DynamicBuffer ? VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC :
			VK_DESCRIPTOR_TYPE_SAMPLER;
	}

	VkDescriptorSetLayoutCreateInfo vkCreateInfo{ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
	vkCreateInfo.bindingCount = (uint32_t)vkBindings.size();
	vkCreateInfo.pBindings = vkBindings.data();

	vkCreateInfo.flags = 0;
	if (shouldUpdateAfterBind)
		vkCreateInfo.flags |= VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT;



	VkDescriptorSetLayoutBindingFlagsCreateInfo bindingFlagsInfo{ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO };
	bindingFlagsInfo.bindingCount = (uint32_t)vkBindingFlags.size();
	bindingFlagsInfo.pBindingFlags = vkBindingFlags.data();
	
	vkCreateInfo.pNext = &bindingFlagsInfo;

	VkResult result = vkCreateDescriptorSetLayout(m_renderContextVk->m_vkDevice, &vkCreateInfo, nullptr, &m_vkHandle);
	if (result != VK_SUCCESS)
	{
		LogError("vkCreateDescriptorSetLayout failed: {}", int32_t(result));
	}
}

e2::IDescriptorSetLayout_Vk::~IDescriptorSetLayout_Vk()
{
	vkDestroyDescriptorSetLayout(m_renderContextVk->m_vkDevice, m_vkHandle, nullptr);
}

