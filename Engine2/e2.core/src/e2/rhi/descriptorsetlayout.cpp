
#include "e2/rhi/descriptorsetlayout.hpp"

e2::IDescriptorSetLayout::IDescriptorSetLayout(e2::IRenderContext* renderContext, DescriptorSetLayoutCreateInfo const& createInfo)
	: e2::RenderResource(renderContext)
{

}

e2::IDescriptorSetLayout::~IDescriptorSetLayout()
{

}
