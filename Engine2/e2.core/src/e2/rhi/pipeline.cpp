
#include "e2/rhi/pipeline.hpp"

#include "e2/rhi/shader.hpp"

e2::IPipeline::IPipeline(e2::IRenderContext* renderContext, PipelineCreateInfo const& createInfo)
	: e2::RenderResource(renderContext)
{

}

e2::IPipeline::~IPipeline()
{

}

e2::IDescriptorSet::IDescriptorSet(e2::IDescriptorPool* descriptorPool, IDescriptorSetLayout* setLayout)
	: e2::ThreadResource(descriptorPool->threadContext())
{

}

e2::IDescriptorSet::~IDescriptorSet()
{

}

e2::IPipelineLayout::IPipelineLayout(e2::IRenderContext* context, e2::PipelineLayoutCreateInfo const& createInfo)
	: e2::RenderResource(context)
{

}

e2::IPipelineLayout::~IPipelineLayout()
{

}
