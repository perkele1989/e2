
#pragma once 

#include <e2/export.hpp>
#include <e2/rhi/renderresource.hpp>
#include <e2/rhi/enums.hpp>
#include <glm/glm.hpp>

#include <cstdint>

namespace e2
{
	struct E2_API SamplerCreateInfo
	{
		// DON'T add more to this than strictly needed.
		// No, we do not need to specify anisotropic level (16x has been standard for decades)
		// No, we likely do not need more than repeat and clamp for wrap options
		// This is highly optimized for Sampler caching.
		
		e2::SamplerFilter filter { e2::SamplerFilter::Anisotropic };
		e2::SamplerWrap wrap { e2::SamplerWrap::Repeat };
	};

	class E2_API ISampler : public e2::RenderResource
	{
		ObjectDeclaration()
	public:
		ISampler(IRenderContext* context, SamplerCreateInfo const& createInfo);
		virtual ~ISampler();
	};



	struct E2_API TextureCreateInfo
	{
		glm::uvec3 resolution {1, 1, 1};
		e2::TextureFormat format { e2::TextureFormat::RGBA8 };
		e2::TextureType type { e2::TextureType::Texture2D };
		e2::TextureLayout initialLayout { e2::TextureLayout::ShaderRead };

		uint32_t arrayLayers{ 1 };
		uint32_t mips { 1 };
	};


	class E2_API ITexture : public e2::RenderResource
	{
		ObjectDeclaration()
	public:
		ITexture(IRenderContext* context, TextureCreateInfo const& createInfo);
		virtual ~ITexture();

		virtual void upload(uint32_t mip, glm::uvec3 offset, glm::uvec3 size, uint8_t const* data, uint64_t dataSize) = 0;

		virtual void generateMips() = 0;
	};
}

#include "texture.generated.hpp"