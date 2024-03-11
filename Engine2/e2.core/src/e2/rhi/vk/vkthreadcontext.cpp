
#include "e2/rhi/vk/vkthreadcontext.hpp"
#include "e2/rhi/vk/vkrendercontext.hpp"
#include "e2/rhi/vk/vkwindow.hpp"
#include "e2/rhi/vk/vkrendertarget.hpp"
#include "e2/rhi/vk/vkpipeline.hpp"
#include "e2/rhi/vk/vktexture.hpp"
#include "e2/rhi/vk/vkvertexlayout.hpp"
#include "e2/rhi/vk/vkdatabuffer.hpp"
#include "e2/rhi/vk/vkdescriptorsetlayout.hpp"

e2::IThreadContext_Vk::IThreadContext_Vk(IRenderContext* context, e2::ThreadContextCreateInfo const& createInfo)
	: e2::IThreadContext(context, createInfo)
	, e2::ContextHolder_Vk(context)
{
	m_threadInfo = e2::threadInfo();

	getPersistentPool();
}

e2::IThreadContext_Vk::~IThreadContext_Vk()
{
	if (m_persistentPool)
	{
		e2::destroy(m_persistentPool);
		m_persistentPool = nullptr;
	}
}

e2::IWindow* e2::IThreadContext_Vk::createWindow(e2::WindowCreateInfo const& createInfo)
{
	return e2::create<IWindow_Vk>(this, createInfo);
}

e2::ICommandPool *e2::IThreadContext_Vk::createCommandPool(e2::CommandPoolCreateInfo const& createInfo)
{
#if defined(E2_DEVELOPMENT)
	if (std::this_thread::get_id() != m_threadInfo.cppId)
	{
		LogWarning("Thread context mismatch.");
	}
#endif

	return e2::create<e2::ICommandPool_Vk>(this, createInfo);
}

e2::IDescriptorPool *e2::IThreadContext_Vk::createDescriptorPool(e2::DescriptorPoolCreateInfo const& createInfo)
{
#if defined(E2_DEVELOPMENT)
	if (std::this_thread::get_id() != m_threadInfo.cppId)
	{
		LogWarning("Thread context mismatch.");
	}
#endif

	return e2::create<e2::IDescriptorPool_Vk>(this, createInfo);
}

e2::ICommandPool* e2::IThreadContext_Vk::getPersistentPool()
{
#if defined(E2_DEVELOPMENT)
	if (std::this_thread::get_id() != m_threadInfo.cppId)
	{
		LogWarning("Thread context mismatch.");
	}
#endif

	if (!m_persistentPool)
	{
		e2::CommandPoolCreateInfo createInfo;
		createInfo.enableReset = true;
		m_persistentPool = e2::create<e2::ICommandPool_Vk>(this, createInfo);
	}
	return m_persistentPool;
}

e2::ICommandPool_Vk::ICommandPool_Vk(IThreadContext* context, CommandPoolCreateInfo const& createInfo)
	: e2::ICommandPool(context, createInfo)
	, e2::ThreadHolder_Vk(context)
{
	VkCommandPoolCreateInfo vkCreateInfo{ VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO };
	vkCreateInfo.queueFamilyIndex = m_renderContextVk->m_queueFamily;

	if (createInfo.enableReset)
		vkCreateInfo.flags |= VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	
	if (createInfo.transient)
		vkCreateInfo.flags |= VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
		
	VkResult result = vkCreateCommandPool(m_renderContextVk->m_vkDevice, &vkCreateInfo, nullptr, &m_vkHandle);
	if (result != VK_SUCCESS)
	{
		LogError("vkCreateCommandPool failed: {}", int32_t(result));
	}
}

e2::ICommandPool_Vk::~ICommandPool_Vk()
{
	vkDestroyCommandPool(m_renderContextVk->m_vkDevice, m_vkHandle, nullptr);
}

void e2::ICommandPool_Vk::reset()
{
	vkResetCommandPool(m_renderContextVk->m_vkDevice, m_vkHandle, 0);
}

e2::ICommandBuffer* e2::ICommandPool_Vk::createBuffer(CommandBufferCreateInfo const& createInfo)
{
	return e2::create<e2::ICommandBuffer_Vk>(m_threadContext, this, createInfo);
}

e2::IDescriptorPool_Vk::IDescriptorPool_Vk(IThreadContext* context, DescriptorPoolCreateInfo const& createInfo)
	: e2::IDescriptorPool(context, createInfo)
	, e2::ThreadHolder_Vk(context)
{
	VkDescriptorPoolCreateInfo vkCreateInfo{VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};
	vkCreateInfo.maxSets = createInfo.maxSets;

	e2::StackVector<VkDescriptorPoolSize, 5> poolSizes;

	if (createInfo.numSamplers > 0)
	{
		poolSizes.resize(poolSizes.size() + 1);
		VkDescriptorPoolSize &newPoolSize = poolSizes[poolSizes.size() - 1];

		newPoolSize.type = VK_DESCRIPTOR_TYPE_SAMPLER;
		newPoolSize.descriptorCount = createInfo.numSamplers;
	}

	if (createInfo.numTextures > 0)
	{
		poolSizes.resize(poolSizes.size() + 1);
		VkDescriptorPoolSize& newPoolSize = poolSizes[poolSizes.size() - 1];

		newPoolSize.type = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
		newPoolSize.descriptorCount = createInfo.numTextures;
	}

	if (createInfo.numUniformBuffers > 0)
	{
		poolSizes.resize(poolSizes.size() + 1);
		VkDescriptorPoolSize& newPoolSize = poolSizes[poolSizes.size() - 1];

		newPoolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		newPoolSize.descriptorCount = createInfo.numUniformBuffers;
	}

	if (createInfo.numDynamicBuffers > 0)
	{
		poolSizes.resize(poolSizes.size() + 1);
		VkDescriptorPoolSize& newPoolSize = poolSizes[poolSizes.size() - 1];

		newPoolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
		newPoolSize.descriptorCount = createInfo.numDynamicBuffers;
	}

	if (createInfo.numStorageBuffers > 0)
	{
		poolSizes.resize(poolSizes.size() + 1);
		VkDescriptorPoolSize& newPoolSize = poolSizes[poolSizes.size() - 1];

		newPoolSize.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		newPoolSize.descriptorCount = createInfo.numStorageBuffers;
	}

	vkCreateInfo.pPoolSizes = poolSizes.data();
	vkCreateInfo.poolSizeCount = (uint32_t)poolSizes.size();
	vkCreateInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;

	if (createInfo.allowUpdateAfterBind)
		vkCreateInfo.flags |= VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT;

	VkResult result = vkCreateDescriptorPool(m_renderContextVk->m_vkDevice, &vkCreateInfo, nullptr, &m_vkHandle);
	if (result != VK_SUCCESS)
	{
		LogError("vkCreateDescriptorPool failed: {}", int32_t(result));
	}
}

e2::IDescriptorPool_Vk::~IDescriptorPool_Vk()
{
	vkDestroyDescriptorPool(m_renderContextVk->m_vkDevice, m_vkHandle, nullptr);
}

e2::IDescriptorSet* e2::IDescriptorPool_Vk::createDescriptorSet(IDescriptorSetLayout* setLayout)
{
	return e2::create<e2::IDescriptorSet_Vk>(this, setLayout);
}

e2::ICommandBuffer_Vk::ICommandBuffer_Vk(IThreadContext* context, e2::ICommandPool_Vk *commandPool, CommandBufferCreateInfo const& createInfo)
	: e2::ICommandBuffer(context, createInfo)
	, e2::ThreadHolder_Vk(context)
{
	m_pool = commandPool;

	VkCommandBufferAllocateInfo vkAllocateInfo{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
	vkAllocateInfo.commandBufferCount = 1;
	vkAllocateInfo.commandPool = m_pool->m_vkHandle;
	vkAllocateInfo.level = createInfo.secondary ? VK_COMMAND_BUFFER_LEVEL_SECONDARY : VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	VkResult result = vkAllocateCommandBuffers(m_renderContextVk->m_vkDevice, &vkAllocateInfo, &m_vkHandle);
	if (result != VK_SUCCESS)
	{
		LogError("vkAllocateCommandBuffers failed: {}", int32_t(result));
	}}

e2::ICommandBuffer_Vk::~ICommandBuffer_Vk()
{
	vkFreeCommandBuffers(m_renderContextVk->m_vkDevice, m_pool->m_vkHandle, 1, &m_vkHandle);
}

void e2::ICommandBuffer_Vk::reset()
{
	vkResetCommandBuffer(m_vkHandle, 0);
}

void e2::ICommandBuffer_Vk::beginRecord(bool oneShot, e2::PipelineSettings const &defaultSettings)
{
	VkCommandBufferBeginInfo beginInfo{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
	
	if(oneShot)
		beginInfo.flags |= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	vkBeginCommandBuffer(m_vkHandle, &beginInfo);

	setCullMode(defaultSettings.cullMode);
	setFrontFace(defaultSettings.frontFace);
	setDepthTest(defaultSettings.depthTest);
	setDepthWrite(defaultSettings.depthWrite);
	setStencilTest(defaultSettings.stencilTest);

	// @todo implement these properly
	vkCmdSetDepthCompareOp(m_vkHandle, VK_COMPARE_OP_LESS);
	vkCmdSetDepthBoundsTestEnable(m_vkHandle, VK_FALSE);
	vkCmdSetStencilOp(m_vkHandle, VK_STENCIL_FACE_FRONT_AND_BACK, VK_STENCIL_OP_ZERO, VK_STENCIL_OP_KEEP, VK_STENCIL_OP_ZERO, VK_COMPARE_OP_EQUAL );
	vkCmdSetDepthBiasEnable(m_vkHandle, VK_FALSE);

	//float blendConstants[4] = {1.0f, 1.0f, 1.0f, 1.0f};
	
	//vkCmdSetBlendConstants(m_vkHandle, blendConstants);
	
}

void e2::ICommandBuffer_Vk::endRecord()
{
	vkEndCommandBuffer(m_vkHandle);
}

void e2::ICommandBuffer_Vk::setDepthTest(bool newDepthTest)
{
	vkCmdSetDepthTestEnable(m_vkHandle, newDepthTest);
}

void e2::ICommandBuffer_Vk::setDepthWrite(bool newDepthWrite)
{
	vkCmdSetDepthWriteEnable(m_vkHandle, newDepthWrite);
}

void e2::ICommandBuffer_Vk::setStencilTest(bool newStencilTest)
{
	vkCmdSetStencilTestEnable(m_vkHandle, newStencilTest);
}

void e2::ICommandBuffer_Vk::setFrontFace(e2::FrontFace newFrontFace)
{
	static auto e2ToVulkan = [](e2::FrontFace ff)  ->VkFrontFace
	{
		switch (ff)
		{
		default:
		case e2::FrontFace::CW:
			return VK_FRONT_FACE_CLOCKWISE;
			break;
		case e2::FrontFace::CCW:
			return VK_FRONT_FACE_COUNTER_CLOCKWISE;
			break;
		}
	};

	vkCmdSetFrontFace(m_vkHandle, e2ToVulkan(newFrontFace));
}

void e2::ICommandBuffer_Vk::setCullMode(e2::CullMode newCullMode)
{
	static auto e2ToVulkan = [] (e2::CullMode cm) ->VkCullModeFlagBits
	{
		switch (cm)
		{
		default:
		case e2::CullMode::None:
			return VK_CULL_MODE_NONE;
			break;
		case e2::CullMode::Front:
			return VK_CULL_MODE_FRONT_BIT;
			break;
		case e2::CullMode::Back:
			return VK_CULL_MODE_BACK_BIT;
			break;
		case e2::CullMode::Both:
			return VK_CULL_MODE_FRONT_AND_BACK;
			break;
		}
	};

	vkCmdSetCullMode(m_vkHandle, e2ToVulkan(newCullMode));
}

void e2::ICommandBuffer_Vk::setScissor(glm::uvec2 offset, glm::uvec2 size)
{
	VkRect2D scissor{
		{(int32_t)offset.x, (int32_t)offset.y},
		{size.x,size.y}
	};
	vkCmdSetScissor(m_vkHandle, 0, 1, &scissor);
}

void e2::ICommandBuffer_Vk::beginRender(e2::IRenderTarget* renderTarget)
{
	e2::IRenderTarget_Vk* vkTarget = static_cast<e2::IRenderTarget_Vk*>(renderTarget);

	m_currentTarget = vkTarget;

	VkViewport viewport{
		.x = 0.0f,
		.y = 0.0f,
		.width = (float)vkTarget->m_vkRenderInfo.renderArea.extent.width,
		.height = (float)vkTarget->m_vkRenderInfo.renderArea.extent.height,
		.minDepth = 0.0f,
		.maxDepth = 1.0f
	};

	vkCmdSetViewport(m_vkHandle, 0, 1, &viewport);
	VkRect2D scissor{
		{0, 0},
		{vkTarget->m_vkRenderInfo.renderArea.extent.width,vkTarget->m_vkRenderInfo.renderArea.extent.height}
	};
	vkCmdSetScissor(m_vkHandle, 0, 1, &scissor);

	vkCmdBeginRendering(m_vkHandle, &vkTarget->m_vkRenderInfo);
	// @todo should we maybe automatically transfer rendertarget attachments to attachment write here? and then back to shader read in endrender? Would be lit 
}

void e2::ICommandBuffer_Vk::endRender()
{
	vkCmdEndRendering(m_vkHandle);
	m_currentTarget = nullptr;
}

void e2::ICommandBuffer_Vk::clearColor(uint32_t attachmentIndex, glm::vec4 const& color)
{
	VkClearAttachment inf{};
	inf.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	inf.colorAttachment = attachmentIndex;
	inf.clearValue.color.float32[0] = color.r;
	inf.clearValue.color.float32[1] = color.g;
	inf.clearValue.color.float32[2] = color.b;
	inf.clearValue.color.float32[3] = color.a;

	VkClearRect rect{};
	rect.baseArrayLayer = 0;
	rect.layerCount = 1;
	rect.rect = m_currentTarget->m_vkRenderInfo.renderArea;

	vkCmdClearAttachments(m_vkHandle, 1, &inf, 1, &rect);
}

void e2::ICommandBuffer_Vk::clearDepth(float depth)
{
	VkClearAttachment inf{};
	inf.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
	inf.clearValue.depthStencil.depth = depth;

	VkClearRect rect{};
	rect.baseArrayLayer = 0;
	rect.layerCount = 1;
	rect.rect = m_currentTarget->m_vkRenderInfo.renderArea;

	vkCmdClearAttachments(m_vkHandle, 1, &inf, 1, &rect);
}

void e2::ICommandBuffer_Vk::clearStencil(uint32_t stencil)
{
	VkClearAttachment inf{};
	inf.aspectMask = VK_IMAGE_ASPECT_STENCIL_BIT;

	inf.clearValue.depthStencil.stencil = stencil;

	VkClearRect rect{};
	rect.baseArrayLayer = 0;
	rect.layerCount = 1;
	rect.rect = m_currentTarget->m_vkRenderInfo.renderArea;

	vkCmdClearAttachments(m_vkHandle, 1, &inf, 1, &rect);
}

void e2::ICommandBuffer_Vk::clearColor(uint32_t attachmentIndex, glm::ivec4 const& color)
{
	VkClearAttachment inf{};
	inf.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	inf.colorAttachment = attachmentIndex;
	inf.clearValue.color.int32[0] = color.r;
	inf.clearValue.color.int32[1] = color.g;
	inf.clearValue.color.int32[2] = color.b;
	inf.clearValue.color.int32[3] = color.a;

	VkClearRect rect{};
	rect.baseArrayLayer = 0;
	rect.layerCount = 1;
	rect.rect = m_currentTarget->m_vkRenderInfo.renderArea;

	vkCmdClearAttachments(m_vkHandle, 1, &inf, 1, &rect);
}

void e2::ICommandBuffer_Vk::clearColor(uint32_t attachmentIndex, glm::uvec4 const& color)
{
	VkClearAttachment inf{};
	inf.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	inf.colorAttachment = attachmentIndex;
	inf.clearValue.color.uint32[0] = color.r;
	inf.clearValue.color.uint32[1] = color.g;
	inf.clearValue.color.uint32[2] = color.b;
	inf.clearValue.color.uint32[3] = color.a;

	VkClearRect rect{};
	rect.baseArrayLayer = 0;
	rect.layerCount = 1;
	rect.rect = m_currentTarget->m_vkRenderInfo.renderArea;

	vkCmdClearAttachments(m_vkHandle, 1, &inf, 1, &rect);
}

void e2::ICommandBuffer_Vk::bindPipeline(e2::IPipeline* pipeline)
{
	e2::IPipeline_Vk* vkPipe = static_cast<e2::IPipeline_Vk*>(pipeline);
	vkCmdBindPipeline(m_vkHandle, VK_PIPELINE_BIND_POINT_GRAPHICS, vkPipe->m_vkHandle);
}

void e2::ICommandBuffer_Vk::bindDescriptorSet(e2::IPipelineLayout* layout, uint32_t setIndex, e2::IDescriptorSet* descriptorSet, uint32_t numDynamicOffsets, uint32_t* dynamicOffsets)
{
	e2::IPipelineLayout_Vk* vkLayout = static_cast<e2::IPipelineLayout_Vk*>(layout);
	e2::IDescriptorSet_Vk* vkSet = static_cast<e2::IDescriptorSet_Vk*>(descriptorSet);
	vkCmdBindDescriptorSets(m_vkHandle, VK_PIPELINE_BIND_POINT_GRAPHICS, vkLayout->m_vkHandle, setIndex, 1, &vkSet->m_vkHandle, numDynamicOffsets, dynamicOffsets);
}

void e2::ICommandBuffer_Vk::nullVertexLayout()
{
	vkCmdSetVertexInputEXT(m_vkHandle, 0, nullptr, 0, nullptr);
}

void e2::ICommandBuffer_Vk::bindVertexLayout(e2::IVertexLayout* vertexLayout)
{
	e2::IVertexLayout_Vk* vkLayout = static_cast<e2::IVertexLayout_Vk*>(vertexLayout);
	vkCmdSetVertexInputEXT(m_vkHandle, (uint32_t)vkLayout->m_vkBindings.size(), vkLayout->m_vkBindings.data(), (uint32_t)vkLayout->m_vkAttributes.size(), vkLayout->m_vkAttributes.data());
}

void e2::ICommandBuffer_Vk::bindVertexBuffer(uint32_t binding, e2::IDataBuffer* dataBuffer)
{
	e2::IDataBuffer_Vk* vkBuffer = static_cast<e2::IDataBuffer_Vk*>(dataBuffer);
	static VkDeviceSize deviceSize{0};
	vkCmdBindVertexBuffers(m_vkHandle, binding, 1, &vkBuffer->m_vkHandle, &deviceSize);
}

void e2::ICommandBuffer_Vk::bindIndexBuffer(e2::IDataBuffer* dataBuffer)
{
	e2::IDataBuffer_Vk* vkBuffer = static_cast<e2::IDataBuffer_Vk*>(dataBuffer);
	vkCmdBindIndexBuffer(m_vkHandle, vkBuffer->m_vkHandle, 0, VK_INDEX_TYPE_UINT32); // @todo support uint16
}

void e2::ICommandBuffer_Vk::pushConstants(e2::IPipelineLayout* layout, uint32_t offset, uint32_t size, uint8_t const* data)
{
	e2::IPipelineLayout_Vk* vkLayout = static_cast<e2::IPipelineLayout_Vk*>(layout);
	vkCmdPushConstants(m_vkHandle, vkLayout->m_vkHandle, VK_SHADER_STAGE_ALL, offset, size, data);
}

void e2::ICommandBuffer_Vk::draw(uint32_t indexCount, uint32_t instanceCount)
{
	vkCmdDrawIndexed(m_vkHandle, indexCount, instanceCount, 0, 0, 0);
}

void e2::ICommandBuffer_Vk::drawNonIndexed(uint32_t vertexCount, uint32_t instanceCount)
{
	vkCmdDraw(m_vkHandle, vertexCount, instanceCount, 0, 0);
}

void e2::ICommandBuffer_Vk::useAsDescriptor(e2::ITexture* texture)
{
	e2::ITexture_Vk* vulkanTexture = texture->unsafeCast<e2::ITexture_Vk>();
	vulkanTexture->m_vkTempLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	m_renderContextVk->vkCmdTransitionImage(m_vkHandle, vulkanTexture->m_vkImage, 1, 0, 1, vulkanTexture->m_vkAspectFlags, vulkanTexture->m_vkLayout, vulkanTexture->m_vkTempLayout);
}


void e2::ICommandBuffer_Vk::useAsDepthStencilAttachment(e2::ITexture* texture)
{
	e2::ITexture_Vk* vulkanTexture = texture->unsafeCast<e2::ITexture_Vk>();
	VkImageLayout newLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
	m_renderContextVk->vkCmdTransitionImage(m_vkHandle, vulkanTexture->m_vkImage, 1, 0, 1, vulkanTexture->m_vkAspectFlags, vulkanTexture->m_vkTempLayout, newLayout);
	vulkanTexture->m_vkTempLayout = newLayout;
}

void e2::ICommandBuffer_Vk::useAsDepthAttachment(e2::ITexture* texture)
{
	e2::ITexture_Vk* vulkanTexture = texture->unsafeCast<e2::ITexture_Vk>();
	VkImageLayout newLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
	m_renderContextVk->vkCmdTransitionImage(m_vkHandle, vulkanTexture->m_vkImage, 1, 0, 1, vulkanTexture->m_vkAspectFlags, vulkanTexture->m_vkTempLayout, newLayout);
	vulkanTexture->m_vkTempLayout = newLayout;

}

void e2::ICommandBuffer_Vk::useAsAttachment(e2::ITexture* texture)
{
	e2::ITexture_Vk* vulkanTexture = texture->unsafeCast<e2::ITexture_Vk>();
	VkImageLayout newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	m_renderContextVk->vkCmdTransitionImage(m_vkHandle, vulkanTexture->m_vkImage, 1, 0, 1, vulkanTexture->m_vkAspectFlags, vulkanTexture->m_vkTempLayout, newLayout);
	vulkanTexture->m_vkTempLayout = newLayout;
}

void e2::ICommandBuffer_Vk::useAsDefault(e2::ITexture* texture)
{
	e2::ITexture_Vk* vulkanTexture = texture->unsafeCast<e2::ITexture_Vk>();
	VkImageLayout newLayout = vulkanTexture->m_vkLayout;
	m_renderContextVk->vkCmdTransitionImage(m_vkHandle, vulkanTexture->m_vkImage, 1, 0, 1, vulkanTexture->m_vkAspectFlags, vulkanTexture->m_vkTempLayout, newLayout);
	vulkanTexture->m_vkTempLayout = newLayout;
}

void e2::ICommandBuffer_Vk::useAsTransferDst(e2::ITexture* texture)
{
	e2::ITexture_Vk* vulkanTexture = texture->unsafeCast<e2::ITexture_Vk>();
	VkImageLayout newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	m_renderContextVk->vkCmdTransitionImage(m_vkHandle, vulkanTexture->m_vkImage, 1, 0, 1, vulkanTexture->m_vkAspectFlags, vulkanTexture->m_vkTempLayout, newLayout);
	vulkanTexture->m_vkTempLayout = newLayout;
}

void e2::ICommandBuffer_Vk::useAsTransferSrc(e2::ITexture* texture)
{
	e2::ITexture_Vk* vulkanTexture = texture->unsafeCast<e2::ITexture_Vk>();
	VkImageLayout newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
	m_renderContextVk->vkCmdTransitionImage(m_vkHandle, vulkanTexture->m_vkImage, 1, 0, 1, vulkanTexture->m_vkAspectFlags, vulkanTexture->m_vkTempLayout, newLayout);
	vulkanTexture->m_vkTempLayout = newLayout;
}

void e2::ICommandBuffer_Vk::blit(e2::ITexture* dst, e2::ITexture* src, uint8_t dstMip, uint8_t srcMip)
{

	e2::ITexture_Vk* vkSrc = src->unsafeCast<e2::ITexture_Vk>();
	glm::uvec2 srcSize = vkSrc->m_size;
	
	e2::ITexture_Vk* vkDst = dst->unsafeCast<e2::ITexture_Vk>();
	glm::uvec2 dstSize = vkDst->m_size;

	for (uint8_t i = 0; i < dstMip; i++)
	{
		if (dstSize.x > 1)
			dstSize.x /= 2;
		if (dstSize.y > 1)
			dstSize.y /= 2;
	}

	for (uint8_t i = 0; i < srcMip; i++)
	{
		if (srcSize.x > 1)
			srcSize.x /= 2;
		if (srcSize.y > 1)
			srcSize.y /= 2;
	}

	// Blit from source to destination
	VkImageBlit blitInfo{};
	blitInfo.srcOffsets[0] = { 0, 0, 0 };
	blitInfo.srcOffsets[1] = { (int32_t)srcSize.x, (int32_t)srcSize.y, 1 };
	blitInfo.srcSubresource.aspectMask = vkSrc->m_vkAspectFlags;
	blitInfo.srcSubresource.mipLevel = srcMip;
	blitInfo.srcSubresource.baseArrayLayer = 0;
	blitInfo.srcSubresource.layerCount = 1;
	blitInfo.dstOffsets[0] = { 0, 0, 0 };
	blitInfo.dstOffsets[1] = { (int32_t)dstSize.x, (int32_t)dstSize.y, 1 };
	blitInfo.dstSubresource.aspectMask = vkDst->m_vkAspectFlags;
	blitInfo.dstSubresource.mipLevel = dstMip;
	blitInfo.dstSubresource.baseArrayLayer = 0;
	blitInfo.dstSubresource.layerCount = 1;
	vkCmdBlitImage(m_vkHandle, vkSrc->m_vkImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, vkDst->m_vkImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &blitInfo, (vkSrc->m_vkAspectFlags & VK_IMAGE_ASPECT_DEPTH_BIT) == VK_IMAGE_ASPECT_DEPTH_BIT ? VK_FILTER_NEAREST : VK_FILTER_LINEAR);

}

e2::IDescriptorSet_Vk::IDescriptorSet_Vk(e2::IDescriptorPool* descriptorPool, IDescriptorSetLayout* setLayout)
	: e2::IDescriptorSet(descriptorPool, setLayout)
	, e2::ThreadHolder_Vk(descriptorPool->threadContext())
{
	e2::IDescriptorPool_Vk* vkPool = static_cast<e2::IDescriptorPool_Vk*>(descriptorPool);
	e2::IDescriptorSetLayout_Vk* vkLayout = static_cast<e2::IDescriptorSetLayout_Vk*>(setLayout);
	m_vkPool = vkPool->m_vkHandle;

	VkDescriptorSetLayout layout = vkLayout->m_vkHandle;

	VkDescriptorSetAllocateInfo allocateInfo{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
	allocateInfo.descriptorPool = m_vkPool;
	allocateInfo.descriptorSetCount = 1;
	allocateInfo.pSetLayouts = &layout;

	VkResult result = vkAllocateDescriptorSets(m_renderContextVk->m_vkDevice, &allocateInfo, &m_vkHandle);
	if (result != VK_SUCCESS)
	{
		LogError("vkAllocateDescriptorSets failed: {}", int32_t(result));
	}
}

e2::IDescriptorSet_Vk::~IDescriptorSet_Vk()
{
	vkFreeDescriptorSets(m_renderContextVk->m_vkDevice, m_vkPool, 1, &m_vkHandle);
}

void e2::IDescriptorSet_Vk::writeSampler(uint32_t binding, e2::ISampler* sampler)
{
	e2::ISampler_Vk* vkSampler = static_cast<e2::ISampler_Vk*>(sampler);
	VkDescriptorImageInfo imageInfo{};
	imageInfo.sampler = vkSampler->m_vkHandle;
	VkWriteDescriptorSet write{ VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
	write.descriptorCount = 1;
	write.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
	write.dstBinding = binding;
	write.dstSet = m_vkHandle;
	write.pImageInfo = &imageInfo;
	vkUpdateDescriptorSets(m_renderContextVk->m_vkDevice, 1, &write, 0, nullptr);
}

void e2::IDescriptorSet_Vk::writeUniformBuffer(uint32_t binding, e2::IDataBuffer* uniformBuffer, uint32_t size, uint32_t offset)
{
	e2::IDataBuffer_Vk* vkBuffer = static_cast<e2::IDataBuffer_Vk*>(uniformBuffer);
	VkDescriptorBufferInfo bufferInfo{};
	bufferInfo.buffer = vkBuffer->m_vkHandle;
	bufferInfo.offset = offset;
	bufferInfo.range = size;

	VkWriteDescriptorSet write{ VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
	write.descriptorCount = 1;
	write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	write.dstBinding = binding;
	write.dstSet = m_vkHandle;
	write.pBufferInfo = &bufferInfo;
	vkUpdateDescriptorSets(m_renderContextVk->m_vkDevice, 1, &write, 0, nullptr);
}

void e2::IDescriptorSet_Vk::writeDynamicBuffer(uint32_t binding, e2::IDataBuffer* dynamicBuffer, uint32_t constantSize, uint32_t constantOffset)
{
	e2::IDataBuffer_Vk* vkBuffer = static_cast<e2::IDataBuffer_Vk*>(dynamicBuffer);
	VkDescriptorBufferInfo bufferInfo{};
	bufferInfo.buffer = vkBuffer->m_vkHandle;
	bufferInfo.offset = constantOffset;
	bufferInfo.range = constantSize;

	VkWriteDescriptorSet write{ VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
	write.descriptorCount = 1;
	write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
	write.dstBinding = binding;
	write.dstSet = m_vkHandle;
	write.pBufferInfo = &bufferInfo;
	vkUpdateDescriptorSets(m_renderContextVk->m_vkDevice, 1, &write, 0, nullptr);
}

void e2::IDescriptorSet_Vk::writeStorageBuffer(uint32_t binding, e2::IDataBuffer* storageBuffer, uint32_t size, uint32_t offset)
{
	e2::IDataBuffer_Vk* vkBuffer = static_cast<e2::IDataBuffer_Vk*>(storageBuffer);
	VkDescriptorBufferInfo bufferInfo{};
	bufferInfo.buffer = vkBuffer->m_vkHandle;
	bufferInfo.offset = offset;
	bufferInfo.range = size;

	VkWriteDescriptorSet write{ VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
	write.descriptorCount = 1;
	write.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	write.dstBinding = binding;
	write.dstSet = m_vkHandle;
	write.pBufferInfo = &bufferInfo;
	vkUpdateDescriptorSets(m_renderContextVk->m_vkDevice, 1, &write, 0, nullptr);
}

namespace
{
	thread_local struct
	{
		bool initialized{};
		std::vector<VkDescriptorImageInfo> data;
	} imageInfo;
	 
}

void e2::IDescriptorSet_Vk::writeTexture(uint32_t binding, e2::ITexture* textures, uint32_t elementOffset, uint32_t numTextures)
{
	if (numTextures == 0)
		return;

	// shortpath for the common usecase where we only have a single texture
	if (numTextures == 1)
	{
		e2::ITexture_Vk* vkTexture = static_cast<e2::ITexture_Vk*>(textures);

		VkDescriptorImageInfo imgInf{};
		imgInf.imageLayout = VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL;
		imgInf.imageView = vkTexture->m_vkImageView;

		VkWriteDescriptorSet write{ VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
		write.descriptorCount = numTextures;
		write.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
		write.dstArrayElement = elementOffset;
		write.dstBinding = binding;
		write.dstSet = m_vkHandle;
		write.pImageInfo = &imgInf;
		vkUpdateDescriptorSets(m_renderContextVk->m_vkDevice, 1, &write, 0, nullptr);

		//if (vkTexture && vkTexture->m_vkImage)
		//{
		//	m_textureCache.addUnique(vkTexture->m_vkImage)
		//}
		//else
		//{
		//	m_textureCache.removeFirstByValueFast(vkTexture)
		//}

		return;
	}

	if (!::imageInfo.initialized)
	{
		uint32_t reserveCount = (glm::max)(numTextures, uint32_t(1024));
		::imageInfo.data.reserve(reserveCount);
		::imageInfo.initialized = true;
	}

	for (uint32_t i = 0; i < numTextures; i++)
	{
		e2::ITexture_Vk* vkTexture = static_cast<e2::ITexture_Vk*>(&textures[i]);
		::imageInfo.data[i].imageView = vkTexture->m_vkImageView;
		::imageInfo.data[i].imageLayout = VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL;
	}

	VkWriteDescriptorSet write{ VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
	write.descriptorCount = numTextures;
	write.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
	write.dstBinding = binding;
	write.dstSet = m_vkHandle;
	write.dstArrayElement = elementOffset;
	write.pImageInfo = &imageInfo.data[0];
	vkUpdateDescriptorSets(m_renderContextVk->m_vkDevice, 1, &write, 0, nullptr);
}
