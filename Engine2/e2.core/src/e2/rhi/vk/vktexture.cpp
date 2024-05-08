
#include "e2/rhi/vk/vktexture.hpp"

#include "e2/rhi/vk/vkrendercontext.hpp"
#include "e2/rhi/vk/vkthreadcontext.hpp"

namespace
{
	VkImageLayout e2ToVk(e2::TextureLayout layout)
	{
		switch (layout)
		{
		default:
		case e2::TextureLayout::ShaderRead:
			return VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			break;
		case e2::TextureLayout::ColorAttachment:
			return VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
			break;
		case e2::TextureLayout::DepthAttachment:
			return VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
			break;
		case e2::TextureLayout::StencilAttachment:
			return VK_IMAGE_LAYOUT_STENCIL_ATTACHMENT_OPTIMAL;
			break;
		case e2::TextureLayout::DepthStencilAttachment:
			return VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
			break;
		case e2::TextureLayout::TransferSource:
			return VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
			break;
		case e2::TextureLayout::TransferDestinatinon:
			return VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
			break;
		case e2::TextureLayout::PresentSource:
			return VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
			break;
		}
	}
}

e2::ISampler_Vk::ISampler_Vk(IRenderContext* context, e2::SamplerCreateInfo const& createInfo)
	: e2::ISampler(context, createInfo)
	, e2::ContextHolder_Vk(context)
{
	m_vkHandle = m_renderContextVk->getOrCreateSampler(createInfo.filter, createInfo.wrap);
}

e2::ISampler_Vk::ISampler_Vk(IRenderContext* context, VkSampler wrap)
	: e2::ISampler(context, {})
	, e2::ContextHolder_Vk(context)
	, m_vkHandle(wrap)
{

}

e2::ISampler_Vk::~ISampler_Vk()
{

}

e2::ITexture_Vk::ITexture_Vk(IRenderContext* context, e2::TextureCreateInfo const& createInfo)
	: e2::ITexture(context, createInfo)
	, e2::ContextHolder_Vk(context)
{
	m_vkFormat = e2ToVk(createInfo.format);
	m_vkAspectFlags = e2ToAspectFlags(createInfo.format);
	m_vkLayout = ::e2ToVk(createInfo.initialLayout);
	m_vkTempLayout = m_vkLayout;
	
	m_mipCount = createInfo.mips;
	m_layerCount = createInfo.arrayLayers;
	m_size = createInfo.resolution;

	VkImageCreateInfo vkCreateInfo{ VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO };
	vkCreateInfo.arrayLayers = createInfo.arrayLayers;
	vkCreateInfo.extent.width = createInfo.resolution.x;
	vkCreateInfo.extent.height = createInfo.resolution.y;
	vkCreateInfo.extent.depth = createInfo.resolution.z;

	/*if (createInfo.type == TextureType::TextureCube || createInfo.type == TextureType::TextureCubeArray)
		vkCreateInfo.flags |= VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
	else if (createInfo.type == TextureType::Texture2DArray || createInfo.type == TextureType::Texture2D)
		vkCreateInfo.flags |= VK_IMAGE_CREATE_2D_ARRAY_COMPATIBLE_BIT;*/
	
	vkCreateInfo.format = m_vkFormat;

	if (createInfo.type == TextureType::Texture3D)
		vkCreateInfo.imageType = VK_IMAGE_TYPE_3D;
	else
		vkCreateInfo.imageType = VK_IMAGE_TYPE_2D;

	vkCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	vkCreateInfo.mipLevels = createInfo.mips;
	vkCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	vkCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
	vkCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
	vkCreateInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;

	if (m_vkAspectFlags & VK_IMAGE_ASPECT_COLOR_BIT)
		vkCreateInfo.usage |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

	if (m_vkAspectFlags & (VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT))
		vkCreateInfo.usage |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;

	VmaAllocationCreateInfo vmaCreateInfo{};
	vmaCreateInfo.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;

	VkResult result{};
	result = vmaCreateImage(m_renderContextVk->m_vmaAllocator, &vkCreateInfo, &vmaCreateInfo, &m_vkImage, &m_vmaHandle, nullptr);
	if (result != VK_SUCCESS)
	{
		LogError("vmaCreateImage failed: {}", int32_t(result));
	}

	VkImageViewCreateInfo viewCreateInfo{ VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
	viewCreateInfo.components = { VK_COMPONENT_SWIZZLE_IDENTITY,VK_COMPONENT_SWIZZLE_IDENTITY,VK_COMPONENT_SWIZZLE_IDENTITY,VK_COMPONENT_SWIZZLE_IDENTITY };
	viewCreateInfo.format = m_vkFormat;
	viewCreateInfo.image = m_vkImage;

	if (createInfo.type == TextureType::Texture2D)
		viewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	else if (createInfo.type == TextureType::Texture2DArray)
		viewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D_ARRAY;
	else if (createInfo.type == TextureType::Texture3D)
		viewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_3D;
	else if (createInfo.type == TextureType::TextureCube)
		viewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_CUBE;
	else if (createInfo.type == TextureType::TextureCubeArray)
		viewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_CUBE_ARRAY;

	viewCreateInfo.subresourceRange.aspectMask = m_vkAspectFlags;
	viewCreateInfo.subresourceRange.baseArrayLayer = 0;
	viewCreateInfo.subresourceRange.baseMipLevel = 0;
	viewCreateInfo.subresourceRange.layerCount = createInfo.arrayLayers;
	viewCreateInfo.subresourceRange.levelCount = createInfo.mips;
	
	result = vkCreateImageView(m_renderContextVk->m_vkDevice, &viewCreateInfo, nullptr, &m_vkImageView);
	if (result != VK_SUCCESS)
	{
		LogError("vkCreateImageView failed: {}", int32_t(result));
	}

	m_renderContextVk->submitTransient(true, [this, &createInfo](VkCommandBuffer buffer) {
		//m_renderContextVk->vkCmdTransitionTexture(buffer, this, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
		m_renderContextVk->vkCmdTransitionTexture(buffer, this, VK_IMAGE_LAYOUT_UNDEFINED, m_vkLayout);
	});
}

e2::ITexture_Vk::~ITexture_Vk()
{
	vkDestroyImageView(m_renderContextVk->m_vkDevice, m_vkImageView, nullptr);
	vmaDestroyImage(m_renderContextVk->m_vmaAllocator, m_vkImage, m_vmaHandle);
}

void e2::ITexture_Vk::generateMips()
{
	m_renderContextVk->submitTransient(true, [this](VkCommandBuffer cmdBuffer) {

		mipsCmd(cmdBuffer);
	});
}

void e2::ITexture_Vk::generateMipsCmd(e2::ICommandBuffer* buff)
{
	VkCommandBuffer cmdBuffer  = buff->unsafeCast<e2::ICommandBuffer_Vk>()->m_vkHandle;
	mipsCmd(cmdBuffer);

}

void e2::ITexture_Vk::mipsCmd(VkCommandBuffer buff)
{
	// Transition all mips to TRANSFER_DST

	//buff->useAsTransferDst(this);
	m_renderContextVk->vkCmdTransitionTexture(buff, this, m_vkTempLayout, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

	int32_t mipWidth = m_size.x;
	int32_t mipHeight = m_size.y;

	// Setup barrier base
	VkImageMemoryBarrier barrier{ VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
	barrier.image = m_vkImage;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.subresourceRange.aspectMask = m_vkAspectFlags;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = 1;
	barrier.subresourceRange.levelCount = 1;
	barrier.subresourceRange.baseMipLevel = 0;
	barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
	barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
	barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
	vkCmdPipelineBarrier(buff, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);


	int32_t tgtWidth = mipWidth;
	int32_t tgtHeight = mipHeight;
	// For each mip level from 1..n
	for (uint32_t i = 1; i < m_mipCount; i++)
	{

		if (tgtWidth > 1) tgtWidth /= 2;
		if (tgtHeight > 1) tgtHeight /= 2;
		
		// Blit from source to destination
		VkImageBlit blit{};
		blit.srcOffsets[0] = { 0, 0, 0 };
		blit.srcOffsets[1] = { mipWidth, mipHeight, 1 };
		blit.srcSubresource.aspectMask = m_vkAspectFlags;
		blit.srcSubresource.mipLevel = 0;
		blit.srcSubresource.baseArrayLayer = 0;
		blit.srcSubresource.layerCount = 1;
		blit.dstOffsets[0] = { 0, 0, 0 };
		blit.dstOffsets[1] = { tgtWidth, tgtHeight, 1 };
		blit.dstSubresource.aspectMask = m_vkAspectFlags;
		blit.dstSubresource.mipLevel = i;
		blit.dstSubresource.baseArrayLayer = 0;
		blit.dstSubresource.layerCount = 1;
		vkCmdBlitImage(buff, m_vkImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, m_vkImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &blit, VK_FILTER_LINEAR);


		// Transition target from TRANSFER_DST to default (shader read)
		barrier.subresourceRange.baseMipLevel = i;
		barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		barrier.newLayout = m_vkLayout;
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
		vkCmdPipelineBarrier(buff, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);

	}

	// Transition source from TRANSFER_SRC to default (shader read)
	barrier.subresourceRange.baseMipLevel = 0;
	barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
	barrier.newLayout = m_vkLayout;
	barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
	barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
	vkCmdPipelineBarrier(buff, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);

}

void e2::ITexture_Vk::upload(uint32_t mip, glm::uvec3 offset, glm::uvec3 size, uint8_t const* data, uint64_t dataSize)
{
	VkResult result{};

	// @todo for now we create staging buffers on the fly, which is a slowdown. Figure out a better strategy for this! (lazily-created thread-context buffers that are grown as needed?)
	VkBuffer stagingBuffer{};
	VkBufferCreateInfo stagingCreateInfo{ VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
	stagingCreateInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
	stagingCreateInfo.size = dataSize;
	stagingCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	VmaAllocationCreateInfo stagingAllocationInfo{ };
	stagingAllocationInfo.usage = VMA_MEMORY_USAGE_AUTO_PREFER_HOST;
	stagingAllocationInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT;

	VmaAllocation stagingAllocation;
	VmaAllocationInfo stagingInfo;
	result = vmaCreateBuffer(m_renderContextVk->m_vmaAllocator, &stagingCreateInfo, &stagingAllocationInfo, &stagingBuffer, &stagingAllocation, &stagingInfo);

	void* gpuData = nullptr;
	vmaMapMemory(m_renderContextVk->m_vmaAllocator, stagingAllocation, &gpuData);
	memcpy(gpuData, data, dataSize);
	vmaUnmapMemory(m_renderContextVk->m_vmaAllocator, stagingAllocation);

	VkBufferImageCopy copyRegion = {};
	copyRegion.bufferOffset = 0;
	copyRegion.bufferRowLength = 0;
	copyRegion.bufferImageHeight = 0;
	copyRegion.imageSubresource.aspectMask = m_vkAspectFlags;
	copyRegion.imageSubresource.mipLevel = mip;
	copyRegion.imageSubresource.baseArrayLayer = 0;
	copyRegion.imageSubresource.layerCount = 1;
	copyRegion.imageOffset.x = offset.x;
	copyRegion.imageOffset.y = offset.y;
	copyRegion.imageOffset.z = offset.z;
	copyRegion.imageExtent.width = size.x;
	copyRegion.imageExtent.height = size.y;
	copyRegion.imageExtent.depth = size.z;

	m_renderContextVk->submitTransient(true, [this, &copyRegion, &stagingBuffer, mip](VkCommandBuffer cmdBuffer) {
		m_renderContextVk->vkCmdTransitionImage(cmdBuffer, m_vkImage, 1, mip, 1, m_vkAspectFlags, m_vkTempLayout, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
		vkCmdCopyBufferToImage(cmdBuffer, stagingBuffer, m_vkImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copyRegion);
		m_renderContextVk->vkCmdTransitionImage(cmdBuffer, m_vkImage, 1, mip, 1, m_vkAspectFlags, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, m_vkTempLayout);

	});

	vmaDestroyBuffer(m_renderContextVk->m_vmaAllocator, stagingBuffer, stagingAllocation);
}

namespace
{
	e2::StackVector<VkFormat, uint64_t(e2::TextureFormat::Count)> e2ToVkMap_Format;
	e2::StackVector<VkImageAspectFlags, uint64_t(e2::TextureFormat::Count)> e2ToVkMap_AspectFlags;
}

VkFormat e2::ITexture_Vk::e2ToVk(e2::TextureFormat textureFormat)
{
	if (::e2ToVkMap_Format.size() == 0)
	{
		::e2ToVkMap_Format.resize(uint64_t(e2::TextureFormat::Count));
		::e2ToVkMap_Format[(uint64_t)e2::TextureFormat::Undefined] = VK_FORMAT_UNDEFINED;
		::e2ToVkMap_Format[(uint64_t)e2::TextureFormat::R8] = VK_FORMAT_R8_UNORM;
		::e2ToVkMap_Format[(uint64_t)e2::TextureFormat::RG8] = VK_FORMAT_R8G8_UNORM;
		::e2ToVkMap_Format[(uint64_t)e2::TextureFormat::RGB8] = VK_FORMAT_R8G8B8_UNORM;
		::e2ToVkMap_Format[(uint64_t)e2::TextureFormat::SRGB8] = VK_FORMAT_R8G8B8_SRGB;
		::e2ToVkMap_Format[(uint64_t)e2::TextureFormat::RGBA8] = VK_FORMAT_R8G8B8A8_UNORM;
		::e2ToVkMap_Format[(uint64_t)e2::TextureFormat::SRGB8A8] = VK_FORMAT_R8G8B8A8_SRGB;
		::e2ToVkMap_Format[(uint64_t)e2::TextureFormat::R16] = VK_FORMAT_R16_SFLOAT;
		::e2ToVkMap_Format[(uint64_t)e2::TextureFormat::RG16] = VK_FORMAT_R16G16_SFLOAT;
		::e2ToVkMap_Format[(uint64_t)e2::TextureFormat::RGB16] = VK_FORMAT_R16G16B16_SFLOAT;
		::e2ToVkMap_Format[(uint64_t)e2::TextureFormat::RGBA16] = VK_FORMAT_R16G16B16A16_SFLOAT;
		::e2ToVkMap_Format[(uint64_t)e2::TextureFormat::R32] = VK_FORMAT_R32_SFLOAT;
		::e2ToVkMap_Format[(uint64_t)e2::TextureFormat::RG32] = VK_FORMAT_R32G32_SFLOAT;
		::e2ToVkMap_Format[(uint64_t)e2::TextureFormat::RGB32] = VK_FORMAT_R32G32B32_SFLOAT;
		::e2ToVkMap_Format[(uint64_t)e2::TextureFormat::RGBA32] = VK_FORMAT_R32G32B32A32_SFLOAT;
		::e2ToVkMap_Format[(uint64_t)e2::TextureFormat::D32] = VK_FORMAT_D32_SFLOAT;
		::e2ToVkMap_Format[(uint64_t)e2::TextureFormat::D16] = VK_FORMAT_D16_UNORM;
		::e2ToVkMap_Format[(uint64_t)e2::TextureFormat::D24S8] = VK_FORMAT_D24_UNORM_S8_UINT;
		::e2ToVkMap_Format[(uint64_t)e2::TextureFormat::D32S8] = VK_FORMAT_D32_SFLOAT_S8_UINT;
		::e2ToVkMap_Format[(uint64_t)e2::TextureFormat::S8] = VK_FORMAT_S8_UINT;
	}

	return ::e2ToVkMap_Format[(uint64_t)textureFormat];
}

VkImageAspectFlags e2::ITexture_Vk::e2ToAspectFlags(e2::TextureFormat textureFormat)
{
	if (::e2ToVkMap_AspectFlags.size() == 0)
	{
		::e2ToVkMap_AspectFlags.resize(uint64_t(e2::TextureFormat::Count));
		::e2ToVkMap_AspectFlags[(uint64_t)e2::TextureFormat::R8] = VK_IMAGE_ASPECT_COLOR_BIT;
		::e2ToVkMap_AspectFlags[(uint64_t)e2::TextureFormat::RG8] = VK_IMAGE_ASPECT_COLOR_BIT;
		::e2ToVkMap_AspectFlags[(uint64_t)e2::TextureFormat::RGB8] = VK_IMAGE_ASPECT_COLOR_BIT;
		::e2ToVkMap_AspectFlags[(uint64_t)e2::TextureFormat::SRGB8] = VK_IMAGE_ASPECT_COLOR_BIT;
		::e2ToVkMap_AspectFlags[(uint64_t)e2::TextureFormat::RGBA8] = VK_IMAGE_ASPECT_COLOR_BIT;
		::e2ToVkMap_AspectFlags[(uint64_t)e2::TextureFormat::SRGB8A8] = VK_IMAGE_ASPECT_COLOR_BIT;
		::e2ToVkMap_AspectFlags[(uint64_t)e2::TextureFormat::R16] = VK_IMAGE_ASPECT_COLOR_BIT;
		::e2ToVkMap_AspectFlags[(uint64_t)e2::TextureFormat::RG16] = VK_IMAGE_ASPECT_COLOR_BIT;
		::e2ToVkMap_AspectFlags[(uint64_t)e2::TextureFormat::RGB16] = VK_IMAGE_ASPECT_COLOR_BIT;
		::e2ToVkMap_AspectFlags[(uint64_t)e2::TextureFormat::RGBA16] = VK_IMAGE_ASPECT_COLOR_BIT;
		::e2ToVkMap_AspectFlags[(uint64_t)e2::TextureFormat::R32] = VK_IMAGE_ASPECT_COLOR_BIT;
		::e2ToVkMap_AspectFlags[(uint64_t)e2::TextureFormat::RG32] = VK_IMAGE_ASPECT_COLOR_BIT;
		::e2ToVkMap_AspectFlags[(uint64_t)e2::TextureFormat::RGB32] = VK_IMAGE_ASPECT_COLOR_BIT;
		::e2ToVkMap_AspectFlags[(uint64_t)e2::TextureFormat::RGBA32] = VK_IMAGE_ASPECT_COLOR_BIT;
		::e2ToVkMap_AspectFlags[(uint64_t)e2::TextureFormat::D32] = VK_IMAGE_ASPECT_DEPTH_BIT;
		::e2ToVkMap_AspectFlags[(uint64_t)e2::TextureFormat::D16] = VK_IMAGE_ASPECT_DEPTH_BIT;
		::e2ToVkMap_AspectFlags[(uint64_t)e2::TextureFormat::D24S8] = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
		::e2ToVkMap_AspectFlags[(uint64_t)e2::TextureFormat::D32S8] = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
		::e2ToVkMap_AspectFlags[(uint64_t)e2::TextureFormat::S8] = VK_IMAGE_ASPECT_STENCIL_BIT;
	}

	return ::e2ToVkMap_AspectFlags[(uint64_t)textureFormat];
}
