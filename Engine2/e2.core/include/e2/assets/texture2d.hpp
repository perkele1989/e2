
#pragma once

#include <e2/buildcfg.hpp>
#include <e2/export.hpp>
#include <e2/rhi/texture.hpp>
#include <e2/assets/asset.hpp>

namespace e2
{


	/** @tags(dynamic, arena, arenaSize=e2::maxNumTexture2DAssets) */
	class E2_API Texture2D : public e2::Asset
	{
		ObjectDeclaration()
	public:
		Texture2D();
		virtual ~Texture2D();

		virtual void write(Buffer& destination) const override;
		virtual bool read(Buffer& source) override;

		inline e2::ITexture* handle()
		{
			return m_texture;
		}

		inline glm::uvec2 resolution()
		{
			return m_resolution;
		}

	protected:
		e2::ITexture* m_texture{};

		glm::uvec2 m_resolution;
		uint8_t m_mipLevels{};
		e2::TextureFormat m_format { e2::TextureFormat::RGBA8 };
	};

}

#include "texture2d.generated.hpp"