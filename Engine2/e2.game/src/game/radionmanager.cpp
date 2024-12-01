
#include "game/radionmanager.hpp"
#include "game/entities/radionentity.hpp"

e2::RadionManager::RadionManager(e2::Game* g)
	: m_game(g)
{
}

e2::RadionManager::~RadionManager()
{
}

void e2::RadionManager::update(double seconds)
{
	m_timeAccumulator += seconds;
	double tickRate = 1.0 / 10.0; // 10tps
	while (m_timeAccumulator >= tickRate)
	{
		for (e2::RadionEntity* ent : m_entities)
		{
			ent->radionTick();
		}

		m_timeAccumulator -= tickRate;
	}
}

e2::Game* e2::RadionManager::game()
{
	return m_game;
}

void e2::RadionManager::registerEntity(e2::RadionEntity* ent)
{
	m_entities.insert(ent);
}

void e2::RadionManager::unregisterEntity(e2::RadionEntity* ent)
{
	m_entities.erase(ent);
}

void e2::RadionManager::discoverEntity(e2::Name name)
{
	if (std::find(m_discoveredEntities.begin(), m_discoveredEntities.end(), name) != m_discoveredEntities.end())
		return;

	m_discoveredEntities.push_back(name);
}

uint32_t e2::RadionManager::numDiscoveredEntities()
{
	return m_discoveredEntities.size();
}

e2::Name e2::RadionManager::discoveredEntity(uint32_t index)
{
	if (index >= m_discoveredEntities.size())
		return e2::Name();

	return m_discoveredEntities[index];
}
