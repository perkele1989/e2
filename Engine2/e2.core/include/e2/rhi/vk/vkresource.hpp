
#pragma once 

#include <e2/export.hpp>

namespace e2
{
	class IRenderContext;
	class IRenderContext_Vk;
	class IThreadContext;
	class IThreadContext_Vk;


	class E2_API ContextHolder_Vk
	{
	public:
		ContextHolder_Vk(e2::IRenderContext* ctx);
		virtual ~ContextHolder_Vk();

		IRenderContext_Vk* m_renderContextVk{};
	};

	class E2_API ThreadHolder_Vk : public e2::ContextHolder_Vk
	{
	public:
		ThreadHolder_Vk(e2::IThreadContext* ctx);
		virtual ~ThreadHolder_Vk();

		IThreadContext_Vk* m_threadContextVk{};
	};
}