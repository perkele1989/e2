
#pragma once

#include <e2/buildcfg.hpp>
#include <e2/export.hpp>
#include <e2/assets/texture2d.hpp>

#include <unordered_map>

namespace e2
{
	class Spritesheet;

	struct E2_API Sprite
	{
		e2::Spritesheet* sheet{};
		glm::vec2 offset;
		glm::vec2 size;
	};

	/** @tags(dynamic, arena, arenaSize=e2::maxNumSpritesheetAssets) */
	class E2_API Spritesheet : public e2::Asset
	{
		ObjectDeclaration()
	public:
		Spritesheet() = default;
		virtual ~Spritesheet();

		virtual void write(e2::IStream& destination) const override;
		virtual bool read(e2::IStream& source) override;

		e2::Sprite *getSprite(e2::Name name);

		inline std::unordered_map<e2::Name, e2::Sprite> &spriteMap()
		{
			return m_sprites;
		}

		inline e2::Texture2DPtr texture()
		{
			return m_texture;
		}

	protected:
		e2::Texture2DPtr m_texture{};

		std::unordered_map<e2::Name, e2::Sprite> m_sprites;
	};



}

#include "spritesheet.generated.hpp"