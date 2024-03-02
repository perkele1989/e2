
#include "e2/rhi/rendercontext.hpp"


e2::IRenderContext::IRenderContext(e2::Context* engineContext, e2::Name applicationName)
	: m_engine(engineContext->engine())
{

}

e2::IRenderContext::~IRenderContext()
{

}
#if defined(E2_DEVELOPMENT)
void e2::IRenderContext::printLeaks()
{
	if (m_trackers.size() > 0)
	{
		LogError("{} leaked render resources:", m_trackers.size());
		for (std::pair<e2::RenderResource* const, ResourceTracker>& pair : m_trackers)
		{
			LogError("{} from:", uint64_t(pair.first));
			LogError("{}\n", pair.second.callStack);
		}

	}
}
#endif

e2::Engine* e2::IRenderContext::engine()
{
	return m_engine;
}
