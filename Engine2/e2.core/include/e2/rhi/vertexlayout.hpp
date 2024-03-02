
#pragma once
#include <e2/buildcfg.hpp>
#include <e2/export.hpp>
#include <e2/utils.hpp>
#include <e2/rhi/enums.hpp>
#include <e2/rhi/renderresource.hpp>

namespace e2
{


	struct E2_API VertexLayoutBinding
	{
		uint32_t stride{};
		e2::VertexRate rate{ e2::VertexRate::PerVertex };
	};

	struct E2_API VertexLayoutAttribute
	{
		uint32_t bindingIndex{};
		e2::VertexFormat format{ e2::VertexFormat::Vec4 };
		uint32_t offset{};

	};

	struct E2_API VertexLayoutCreateInfo
	{
		e2::StackVector<VertexLayoutBinding, maxNumVertexBindings> bindings;
		e2::StackVector<VertexLayoutAttribute, maxNumVertexAttributes> attributes;
	};

	class E2_API IVertexLayout : public e2::RenderResource
	{
		ObjectDeclaration()
	public:
		IVertexLayout(IRenderContext* context, VertexLayoutCreateInfo const& createInfo);
		virtual ~IVertexLayout();
	};
}

#include "vertexlayout.generated.hpp"