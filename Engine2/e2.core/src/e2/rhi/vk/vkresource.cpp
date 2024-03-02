
#include "e2/rhi/vk/vkresource.hpp"
#include "e2/rhi/vk/vkrendercontext.hpp"
#include "e2/rhi/vk/vkthreadcontext.hpp"

e2::ContextHolder_Vk::ContextHolder_Vk(e2::IRenderContext* ctx)
{
	m_renderContextVk = static_cast<e2::IRenderContext_Vk*>(ctx);
}

e2::ContextHolder_Vk::~ContextHolder_Vk()
{

}

e2::ThreadHolder_Vk::ThreadHolder_Vk(e2::IThreadContext* ctx)
	: e2::ContextHolder_Vk(ctx->renderContext())
{
	m_threadContextVk = static_cast<e2::IThreadContext_Vk*>(ctx);
}

e2::ThreadHolder_Vk::~ThreadHolder_Vk()
{

}
