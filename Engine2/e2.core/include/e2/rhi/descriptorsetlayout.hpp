

#pragma once


#include <e2/buildcfg.hpp>
#include <e2/export.hpp>
#include <e2/utils.hpp>
#include <e2/rhi/renderresource.hpp>

namespace e2
{

	enum class DescriptorBindingType : uint8_t
	{
		Sampler,
		Texture,
		UniformBuffer,
		DynamicBuffer,
		StorageBuffer
	};

	struct E2_API DescriptorSetBinding
	{
		DescriptorBindingType type{ e2::DescriptorBindingType::UniformBuffer };
		uint32_t count{ 1 };
		bool allowPartiallyBound{false};
		bool updateAfterBind{ false };
	};

	struct E2_API DescriptorSetLayoutCreateInfo
	{
		e2::StackVector<DescriptorSetBinding, maxSetBindings> bindings;
	};

	class E2_API IDescriptorSetLayout : public e2::RenderResource
	{
		ObjectDeclaration()
	public:
		IDescriptorSetLayout(e2::IRenderContext* renderContext, DescriptorSetLayoutCreateInfo const& createInfo);
		virtual ~IDescriptorSetLayout();
	};

}

#include "descriptorsetlayout.generated.hpp"