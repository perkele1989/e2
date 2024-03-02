
#include "e2/rhi/vk/vkdatabuffer.hpp"

#include "e2/rhi/vk/vkrendercontext.hpp"
#include "e2/rhi/vk/vkthreadcontext.hpp"

namespace
{
	//uint64_t numBuffers;
}

e2::IDataBuffer_Vk::IDataBuffer_Vk(IRenderContext* context, e2::DataBufferCreateInfo const& createInfo)
	: e2::IDataBuffer(context, createInfo)
	, e2::ContextHolder_Vk(context)
{
	m_size = createInfo.size;
	m_dynamic = createInfo.dynamic;
	m_type = createInfo.type;

	createVkBuffer();

	//::numBuffers++;
	//LogNotice("Num buffers increased to {}", ::numBuffers);
}

e2::IDataBuffer_Vk::~IDataBuffer_Vk()
{
	destroyVkBuffer();

	//::numBuffers--;
	//LogNotice("Num buffers decreased to {}", ::numBuffers);
}

void e2::IDataBuffer_Vk::download(uint8_t* destination, uint64_t destinationSize, uint64_t sourceSize, uint64_t sourceOffset)
{
	uint64_t size = destinationSize < sourceSize ? destinationSize : sourceSize;

	// quickpath for dynamic buffers
	if (m_dynamic)
	{
		memcpy(destination, reinterpret_cast<uint8_t*>(m_map) + sourceOffset, size);
		return;
	}


	VkResult result{};

	// @todo for now we create staging buffers on the fly, which is a slowdown. Figure out a better strategy for this! (lazily-created thread-context buffers that are grown as needed?)
	VkBuffer stagingBuffer{};
	VkBufferCreateInfo stagingCreateInfo{ VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
	stagingCreateInfo.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT;
	stagingCreateInfo.size = size;
	stagingCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	VmaAllocationCreateInfo stagingAllocationInfo{ };
	stagingAllocationInfo.usage = VMA_MEMORY_USAGE_AUTO_PREFER_HOST;
	stagingAllocationInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT;

	VmaAllocation stagingAllocation;
	VmaAllocationInfo stagingInfo;
	result = vmaCreateBuffer(m_renderContextVk->m_vmaAllocator, &stagingCreateInfo, &stagingAllocationInfo, &stagingBuffer, &stagingAllocation, &stagingInfo);


	m_renderContextVk->submitTransient(true, [this, size, sourceOffset, &stagingBuffer](VkCommandBuffer cmdBuffer) {
		VkBufferCopy region;
		region.dstOffset = 0;
		region.size = size;
		region.srcOffset = sourceOffset;
		vkCmdCopyBuffer(cmdBuffer, m_vkHandle, stagingBuffer,1, &region);
	});


	memcpy(destination, stagingInfo.pMappedData, size);

	vmaDestroyBuffer(m_renderContextVk->m_vmaAllocator, stagingBuffer, stagingAllocation);
}

void e2::IDataBuffer_Vk::upload(uint8_t const* sourceData, uint64_t sourceSize, uint64_t sourceOffset, uint64_t destinationOffset)
{
	// quick path for dynamic buffers 
	if (m_dynamic)
	{
		memcpy(reinterpret_cast<uint8_t*>(m_map) + destinationOffset, sourceData + sourceOffset, sourceSize);
		return;
	}

	VkResult result{};

	// @todo for now we create staging buffers on the fly, which is a slowdown. Figure out a better strategy for this! (lazily-created thread-context buffers that are grown as needed?)
	VkBuffer stagingBuffer{};
	VkBufferCreateInfo stagingCreateInfo{ VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
	stagingCreateInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
	stagingCreateInfo.size = sourceSize;
	stagingCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	VmaAllocationCreateInfo stagingAllocationInfo{ };
	stagingAllocationInfo.usage = VMA_MEMORY_USAGE_AUTO_PREFER_HOST;
	stagingAllocationInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT;
	
	VmaAllocation stagingAllocation;
	VmaAllocationInfo stagingInfo;
	result = vmaCreateBuffer(m_renderContextVk->m_vmaAllocator, &stagingCreateInfo, &stagingAllocationInfo, &stagingBuffer, &stagingAllocation, &stagingInfo);

	memcpy(stagingInfo.pMappedData, sourceData + sourceOffset, sourceSize);

	m_renderContextVk->submitTransient(true, [this, destinationOffset, sourceSize, sourceOffset, &stagingBuffer](VkCommandBuffer cmdBuffer) {
		VkBufferCopy region;
		region.dstOffset = destinationOffset;
		region.size = sourceSize;
		region.srcOffset = sourceOffset;
		vkCmdCopyBuffer(cmdBuffer, stagingBuffer, m_vkHandle, 1, &region);
	});

	vmaDestroyBuffer(m_renderContextVk->m_vmaAllocator, stagingBuffer, stagingAllocation);
}

void e2::IDataBuffer_Vk::createVkBuffer()
{
	if (m_size == 0)
		return;

	VkResult result{};

	VkBufferCreateInfo createInfo{ VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
	createInfo.size = m_size;
	createInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE; // exclusive to this queue family
	
	if (m_type == BufferType::IndexBuffer)
		createInfo.usage |= VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
	else if (m_type == BufferType::UniformBuffer)
		createInfo.usage |= VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
	else if (m_type == BufferType::VertexBuffer)
		createInfo.usage |= VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
	else if (m_type == BufferType::StorageBuffer)
		createInfo.usage |= VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;

	createInfo.usage |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;
	createInfo.usage |= VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

	VmaAllocationInfo allocationInfo;
	VmaAllocationCreateInfo allocationCreateInfo{};
	allocationCreateInfo.usage = VMA_MEMORY_USAGE_AUTO;
	
	// Host_Access_Sequential_Write guarantees HOST_VISIBLE, and HOST_VISIBLE is in practice always HOST_COHERENT  on PC
	if (m_dynamic)
		allocationCreateInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT;

	result = vmaCreateBuffer(m_renderContextVk->m_vmaAllocator, &createInfo, &allocationCreateInfo, &m_vkHandle, &m_vmaHandle, &allocationInfo);
	if (result != VK_SUCCESS)
	{
		LogError("vmaCreateBuffer failed: {}", int32_t(result));
	}

	if (m_dynamic)
	{
		m_map = allocationInfo.pMappedData;
	}
}

void e2::IDataBuffer_Vk::destroyVkBuffer()
{
	if (m_size == 0)
		return;

	vmaDestroyBuffer(m_renderContextVk->m_vmaAllocator, m_vkHandle, m_vmaHandle);
}
