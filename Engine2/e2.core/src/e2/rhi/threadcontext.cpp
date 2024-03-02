
#include "e2/rhi/threadcontext.hpp"

e2::ThreadResource::ThreadResource(IThreadContext* context)
	: e2::RenderResource(context->renderContext())
{
	m_threadContext = context;
}

e2::ThreadResource::~ThreadResource()
{

}

e2::IThreadContext* e2::ThreadResource::threadContext() const
{
	return m_threadContext;
}


e2::IThreadContext::IThreadContext(IRenderContext* context, ThreadContextCreateInfo const& createInfo)
	: e2::RenderResource(context)
{

}

e2::IThreadContext::~IThreadContext()
{

}

e2::ICommandBuffer::ICommandBuffer(IThreadContext* context, CommandBufferCreateInfo const& createInfo)
 : e2::ThreadResource(context)
{

}

e2::ICommandBuffer::~ICommandBuffer()
{

}

e2::ICommandPool::ICommandPool(IThreadContext* context, CommandPoolCreateInfo const& createInfo)
	: e2::ThreadResource(context)
{

}

e2::ICommandPool::~ICommandPool()
{

}

e2::IDescriptorPool::IDescriptorPool(IThreadContext* context, DescriptorPoolCreateInfo const& createInfo)
	: e2::ThreadResource(context)
{

}

e2::IDescriptorPool::~IDescriptorPool()
{

}
