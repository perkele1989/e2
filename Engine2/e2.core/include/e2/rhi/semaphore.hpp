
#pragma once 

#include <e2/export.hpp>

#include <e2/rhi/renderresource.hpp>

#include <glm/glm.hpp> 


namespace e2
{

	struct E2_API SemaphoreCreateInfo
	{
		
	};

	class E2_API ISemaphore : public e2::RenderResource
	{
		ObjectDeclaration()
	public:
		ISemaphore(e2::IRenderContext* renderContext, e2::SemaphoreCreateInfo const& createInfo);
		virtual ~ISemaphore();
	};
}

#include "semaphore.generated.hpp"