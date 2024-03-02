
#pragma once 

#include <e2/buildcfg.hpp>
#include <e2/export.hpp>
#include <e2/utils.hpp>

#include <e2/rhi/enums.hpp>
#include <e2/rhi/renderresource.hpp>

namespace e2
{
	class ITexture;

	struct ClearValue
	{
		union
		{
			glm::vec4 clearColorf32;
			glm::ivec4 clearColori32;
			glm::uvec4 clearColoru32;

			struct
			{
				float depth;
				uint32_t stencil;
				uint32_t padding[2];
			};
		};
	};

	enum class ClearMethod : uint8_t
	{
		ColorFloat,
		ColorUint,
		ColorInt,
		DepthStencil
	};

	struct RenderAttachment
	{
		e2::ITexture* target{};
		e2::ClearMethod clearMethod{ e2::ClearMethod::ColorFloat  };
		e2::LoadOperation loadOperation{ e2::LoadOperation::Ignore };
		e2::StoreOperation storeOperation{ e2::StoreOperation::Store };
		e2::ClearValue clearValue {glm::vec4(0.f, 0.f, 0.f, 0.f)};


	};

	struct E2_API RenderTargetCreateInfo
	{
		glm::uvec2 areaOffset;
		glm::uvec2 areaExtent;
		uint32_t layerCount{ 1 };

		e2::StackVector<RenderAttachment, maxNumRenderAttachments> colorAttachments;
		RenderAttachment depthAttachment; // target is null if unused
		RenderAttachment stencilAttachment; // target is null if unused
	};

	class E2_API IRenderTarget : public e2::RenderResource
	{
		ObjectDeclaration()
	public:
		IRenderTarget(IRenderContext* context, RenderTargetCreateInfo const& createInfo);
		virtual ~IRenderTarget();

	};
}

#include "rendertarget.generated.hpp"