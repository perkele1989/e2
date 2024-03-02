
#include "e2/rhi/databuffer.hpp"

#include <vector>
#include <cstdint>

e2::IDataBuffer::IDataBuffer(IRenderContext* context, e2::DataBufferCreateInfo const& createInfo)
	: e2::RenderResource(context)
{

}

e2::IDataBuffer::~IDataBuffer()
{

}