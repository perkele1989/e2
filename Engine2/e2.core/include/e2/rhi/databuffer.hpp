
#pragma once 

#include <e2/export.hpp>
#include <e2/rhi/renderresource.hpp>
#include <e2/rhi/enums.hpp>

#include <cstdint>

namespace e2
{
	class ICommandBuffer;

	struct E2_API DataBufferCreateInfo
	{
		e2::BufferType type {e2::BufferType::VertexBuffer };
		uint32_t size { 1 };
		bool dynamic{ false };
	};

	class E2_API IDataBuffer : public e2::RenderResource
	{
		ObjectDeclaration()
	public:
		IDataBuffer(IRenderContext* context, e2::DataBufferCreateInfo const& createInfo);
		virtual ~IDataBuffer();

		/** */
		virtual void download(uint8_t* destination, uint64_t destinationSize, uint64_t sourceSize, uint64_t sourceOffset) = 0;

		virtual void upload(uint8_t const* sourceData, uint64_t sourceSize, uint64_t sourceOffset, uint64_t destinationOffset) = 0;
	};
}

#include "databuffer.generated.hpp"