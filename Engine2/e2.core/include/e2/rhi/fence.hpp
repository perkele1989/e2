
#pragma once 

#include <e2/export.hpp>

#include <e2/rhi/renderresource.hpp>

#include <glm/glm.hpp> 


namespace e2
{

	struct E2_API FenceCreateInfo
	{
		bool createSignaled{};
	};

	class E2_API IFence : public e2::RenderResource
	{
		ObjectDeclaration()
	public:
		IFence(e2::IRenderContext* renderContext, e2::FenceCreateInfo const& createInfo);
		virtual ~IFence();

		virtual bool wait(uint64_t nsTime = UINT64_MAX) = 0;
		virtual void reset() = 0;
	};
}

#include "fence.generated.hpp"