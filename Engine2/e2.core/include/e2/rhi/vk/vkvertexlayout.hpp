
#pragma once

#include <e2/buildcfg.hpp>

#include "e2/rhi/vertexlayout.hpp"
#include "e2/rhi/vk/vkresource.hpp"

#include <Volk/volk.h>

namespace e2
{
	/** @tags(arena, arenaSize=e2::maxVkVertexLayouts) */
	class E2_API IVertexLayout_Vk : public e2::IVertexLayout, public e2::ContextHolder_Vk
	{
		ObjectDeclaration()
	public:
		IVertexLayout_Vk(IRenderContext* context, e2::VertexLayoutCreateInfo const& createInfo);
		virtual ~IVertexLayout_Vk();

		e2::StackVector<VkVertexInputBindingDescription2EXT, e2::maxNumVertexBindings> m_vkBindings;
		e2::StackVector<VkVertexInputAttributeDescription2EXT, e2::maxNumVertexAttributes> m_vkAttributes;

	};
}

#include "vkvertexlayout.generated.hpp"