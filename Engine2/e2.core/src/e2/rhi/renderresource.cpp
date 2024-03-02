
#include "e2/rhi/renderresource.hpp"
#include "e2/rhi/rendercontext.hpp"

#include <stacktrace>
#include <sstream>

e2::RenderResource::RenderResource(IRenderContext* context)
	: m_renderContext(context)
{
#if defined(E2_DEVELOPMENT)
	std::stringstream ss;
	//ss << std::stacktrace::current();

	m_renderContext->m_trackers[this] = { this, ss.str() };
#endif
}

e2::RenderResource::~RenderResource()
{
#if defined(E2_DEVELOPMENT)
	m_renderContext->m_trackers.erase(this);
#endif
}

e2::IRenderContext* e2::RenderResource::renderContext() const
{
	return m_renderContext;
}
