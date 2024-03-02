
#include "e2/assets/texture2d.hpp"
#include "e2/rhi/rendercontext.hpp"

#include<glm/gtc/integer.hpp>

namespace
{
	uint64_t numTextures{};
}

e2::Texture2D::Texture2D()
{
	::numTextures++;
	LogNotice("Num textures increased to {}", ::numTextures);
}

e2::Texture2D::~Texture2D()
{
	if(m_texture)
		e2::destroy(m_texture);

	::numTextures--;
	LogNotice("Num textures decreased to {}", ::numTextures);
}



void e2::Texture2D::write(Buffer& destination) const
{

}

bool e2::Texture2D::read(Buffer& source)
{
	if (m_texture)
		e2::destroy(m_texture);

	source >> m_resolution;

	bool loadMips = false;
	bool generateMips = true;
	if (version >= e2::AssetVersion::AddMipsToTexture)
	{
		uint8_t genmips8;
		source >> m_mipLevels;
		source >> genmips8;
		generateMips = genmips8 != 0;
		loadMips = true;
	}
	else
	{
		m_mipLevels = e2::calculateMipLevels(m_resolution);
	}

	

	uint8_t formatInt{};
	source >> formatInt;
	m_format = e2::TextureFormat(formatInt);

	e2::TextureCreateInfo createInfo{};
	createInfo.format = m_format;
	createInfo.mips = m_mipLevels;
	createInfo.resolution = { m_resolution, 1 };
	createInfo.type = TextureType::Texture2D;
	m_texture = renderContext()->createTexture(createInfo);


	if (loadMips)
	{
		glm::uvec2 res = m_resolution;
		for (uint8_t i = 0; i < m_mipLevels; i++)
		{
			uint64_t dataSize{};
			source >> dataSize;
			m_texture->upload(i, glm::uvec3(0, 0, 0), glm::uvec3(res, 1), source.current(), dataSize);
			uint64_t oldCursor;
			source.consume(dataSize, oldCursor);

			res /= 2;
		}
	}
	else
	{
		uint64_t dataSize{};
		source >> dataSize;
		m_texture->upload(0, glm::uvec3(0, 0, 0), glm::uvec3(m_resolution, 1), source.current(), dataSize);
		uint64_t oldCursor;
		source.consume(dataSize, oldCursor);
	}

	if (generateMips)
		m_texture->generateMips();

	return true;
}
