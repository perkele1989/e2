
#include "e2/rhi/semaphore.hpp"

e2::ISemaphore::ISemaphore(e2::IRenderContext* renderContext, e2::SemaphoreCreateInfo const& createInfo)
	: e2::RenderResource(renderContext)
{

}

e2::ISemaphore::~ISemaphore()
{

}
