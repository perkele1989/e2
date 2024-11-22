
#include "game/components/fogcomponent.hpp"
#include "game/entity.hpp"
#include "game/hex.hpp"


namespace
{
	std::vector<e2::Hex> tmpHex;
	void cleanTmpHex()
	{
		tmpHex.clear();
		tmpHex.reserve(16);
	}
}

e2::FogComponent::FogComponent(e2::Entity* entity, uint32_t sightRange, uint32_t extraRange)
	: m_entity(entity)
	, m_sightRange(sightRange)
	, m_extraRange(extraRange)
{
}

e2::FogComponent::~FogComponent()
{
	for (e2::Hex& h : m_visibilityHexes)
	{
		m_entity->hexGrid()->unflagVisible(h.offsetCoords());
	}

}

void e2::FogComponent::refresh()
{

	e2::Hex thisHex = m_entity->hex();
	if (thisHex == m_lastHex)
	{
		return;
	}

	for (e2::Hex& h : m_visibilityHexes)
	{
		m_entity->hexGrid()->unflagVisible(h.offsetCoords());
	}
	m_visibilityHexes.clear();


	::cleanTmpHex();
	e2::Hex::circle(thisHex, m_sightRange + m_extraRange, ::tmpHex);
	for (e2::Hex h : ::tmpHex)
	{
		bool onlyPoke = e2::Hex::distance(h, thisHex) >= m_sightRange;
		m_entity->hexGrid()->flagVisible(h.offsetCoords(), onlyPoke);
		if (!onlyPoke)
		{
			m_visibilityHexes.push_back(h);
		}
	}

	m_lastHex = thisHex;
}
