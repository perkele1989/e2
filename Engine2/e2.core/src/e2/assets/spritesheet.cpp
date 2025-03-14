
#include "e2/assets/spritesheet.hpp"

#include "e2/managers/assetmanager.hpp"

e2::Spritesheet::~Spritesheet()
{

}

void e2::Spritesheet::write(e2::IStream& destination) const
{
	e2::Asset::write(destination);
}

bool e2::Spritesheet::read(e2::IStream& source)
{
	if (!e2::Asset::read(source))
		return false;

	uint32_t numSprites = 0;
	source >> numSprites;
	for (uint32_t i = 0; i < numSprites; i++)
	{
		e2::Name name;
		glm::vec2 offset;
		glm::vec2 size;
		source >> name >> offset >> size;
		
		e2::Sprite newSprite;
		newSprite.sheet = this;
		newSprite.offset = offset;
		newSprite.size = size;

		m_sprites[name] = newSprite;
	}

	if (dependencies.size() == 0)
		return false;

	m_texture = assetManager()->get(dependencies[0].assetName).cast<e2::Texture2D>();

	if (!m_texture)
		return false;

	return true;
}

e2::Sprite* e2::Spritesheet::getSprite(e2::Name name)
{
	auto it = m_sprites.find(name);
	if (it == m_sprites.end())
	{
		//LogError("No such sprite in spritesheet: {}", name);
		return nullptr;
	}

	return &it->second;
}
