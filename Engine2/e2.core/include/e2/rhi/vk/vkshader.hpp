
#pragma once 

#include <e2/buildcfg.hpp>

#include <e2/rhi/shader.hpp>
#include <e2/rhi/vk/vkresource.hpp>

#include <Volk/volk.h>

namespace e2
{
	/** @tags(arena, arenaSize=e2::maxVkShaders) */
	class E2_API IShader_Vk : public e2::IShader, public e2::ContextHolder_Vk
	{
		ObjectDeclaration()
	public:
		IShader_Vk(IRenderContext* context, e2::ShaderCreateInfo const& createInfo);
		virtual ~IShader_Vk();

		VkShaderStageFlagBits m_vkStage;
		VkShaderModule m_vkHandle{};

	};

}

#include "vkshader.generated.hpp"