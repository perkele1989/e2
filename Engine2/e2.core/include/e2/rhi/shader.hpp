
#pragma once 
#include <e2/buildcfg.hpp>
#include <e2/export.hpp>
#include <e2/rhi/enums.hpp>
#include <e2/rhi/renderresource.hpp>
#include <string>
#include <map>

namespace e2
{

	struct E2_API ShaderDefine
	{
		e2::Name name;
		e2::Name value;
	};

	struct E2_API ShaderCreateInfo
	{
		e2::StackVector<e2::ShaderDefine, e2::maxShaderDefines> defines;
		e2::ShaderStage stage{ e2::ShaderStage::Vertex };
		char const* source{};
		
	};

	class E2_API IShader : public e2::RenderResource
	{
		ObjectDeclaration()
	public:
		IShader(IRenderContext* context, e2::ShaderCreateInfo const& createInfo);
		virtual ~IShader();

		virtual bool valid() = 0;
	};
}

#include "shader.generated.hpp"