
#pragma once 

#include <e2/export.hpp>
#include <e2/utils.hpp>

namespace e2
{
	class IRenderContext;

	class E2_API RenderResource : public e2::ManagedObject
	{
		ObjectDeclaration()
	public:
		RenderResource(IRenderContext* context);
		virtual ~RenderResource();

		IRenderContext* renderContext() const;

		template<typename ContextType>
		ContextType* renderContext() const
		{
			return static_cast<ContextType*>(m_renderContext);
		}

	protected:
		IRenderContext* m_renderContext{};
	};

	class IThreadContext;

	class E2_API ThreadResource : public e2::RenderResource
	{
		ObjectDeclaration()
	public:
		ThreadResource(IThreadContext* context);
		virtual ~ThreadResource();

		IThreadContext* threadContext() const;

		template<typename ContextType>
		ContextType* threadContext() const
		{
			return static_cast<ContextType*>(m_threadContext);
		}

	protected:
		IThreadContext* m_threadContext{};
	};


}

#include "renderresource.generated.hpp"