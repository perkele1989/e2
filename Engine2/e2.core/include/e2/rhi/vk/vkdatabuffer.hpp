
#pragma once 

#include <e2/buildcfg.hpp>

#include <e2/rhi/databuffer.hpp>
#include <e2/rhi/vk/vkresource.hpp>

#include <Volk/volk.h>
#include <vma/vk_mem_alloc.h>

namespace e2
{
	/** @tags(arena, arenaSize=e2::maxVkDataBuffers) */
	class E2_API IDataBuffer_Vk : public e2::IDataBuffer, public e2::ContextHolder_Vk
	{
		ObjectDeclaration() 
	public:
		IDataBuffer_Vk(IRenderContext* context, e2::DataBufferCreateInfo const& createInfo);
		virtual ~IDataBuffer_Vk();

		virtual void download(uint8_t* destination, uint64_t destinationSize, uint64_t sourceSize, uint64_t sourceOffset) override;

		virtual void upload(uint8_t const* sourceData, uint64_t sourceSize, uint64_t sourceOffset, uint64_t destinationOffset) override;

		uint64_t m_size{};
		e2::BufferType m_type{};
		bool m_dynamic{};

		VkBuffer m_vkHandle{};
		VmaAllocation m_vmaHandle{};

		void *m_map{};

		bool m_needFlush{};

	private:
		void createVkBuffer();
		void destroyVkBuffer();
	};
}

#include "vkdatabuffer.generated.hpp"