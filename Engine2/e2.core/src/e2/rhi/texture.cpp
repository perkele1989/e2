
#include "e2/rhi/texture.hpp"

e2::ITexture::ITexture(IRenderContext* context, TextureCreateInfo const& createInfo)
	: e2::RenderResource(context)
{

}

e2::ITexture::~ITexture()
{

}

e2::ISampler::ISampler(IRenderContext* context, SamplerCreateInfo const& createInfo)
	: e2::RenderResource(context)
{

}

e2::ISampler::~ISampler()
{

}
