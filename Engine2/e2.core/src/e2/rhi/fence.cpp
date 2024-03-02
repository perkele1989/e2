
#include "e2/rhi/fence.hpp"

e2::IFence::IFence(e2::IRenderContext* renderContext, e2::FenceCreateInfo const& createInfo)
	: e2::RenderResource(renderContext)
{

}

e2::IFence::~IFence()
{

}
