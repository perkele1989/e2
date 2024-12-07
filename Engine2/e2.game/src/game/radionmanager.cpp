
#include "game/radionmanager.hpp"
#include "game/entities/radionentity.hpp"
#include "game/game.hpp"

e2::RadionManager::RadionManager(e2::Game* g)
	: m_game(g)
{
}

e2::RadionManager::~RadionManager()
{
}

void e2::RadionManager::clearDiscovered()
{
	m_discoveredEntities.clear();
}


void e2::RadionManager::tickWithParents(e2::RadionEntity* node)
{
	if (m_tickedNodes.contains(node))
		return;

	m_tickedNodes.insert(node);
	m_untickedNodes.erase(node);

	for (uint32_t i = 0; i < node->radionSpecification->pins.size(); i++)
	{
		e2::RadionPin& pin = node->radionSpecification->pins[i];
		if (pin.type != e2::RadionPinType::Input)
			continue;

		e2::RadionSlot& slot = node->slots[i];

		if (slot.connections.size() > 1)
		{
			LogWarning("More than one connection on input pin");
		}

		for (e2::RadionConnection& conn : slot.connections)
		{
			if (!conn.otherEntity)
				continue;

			tickWithParents(conn.otherEntity);
		}

		//e2::RadionConnection& conn = node->connections[i];


	}

	node->radionTick();

}

void e2::RadionManager::update(double seconds)
{
	m_timeAccumulator += seconds;
	constexpr double tickRate = 1.0 / 10.0; // 10tps

	m_endNodes.clear();

	for (e2::RadionEntity* node : m_entities)
	{
		if (!node->anyOutputConnected())
		{
			m_endNodes.insert(node);
		}
	}

	while (m_timeAccumulator >= tickRate)
	{
		m_tickedNodes.clear();
		m_untickedNodes = m_entities;
		for (e2::RadionEntity* endNode : m_endNodes)
		{
			tickWithParents(endNode);
		}

		while (!m_untickedNodes.empty())
		{
			tickWithParents(*m_untickedNodes.begin());
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

void e2::RadionManager::populatePins(e2::Hex const& coords, e2::RadionPinType type, std::vector<e2::RadionWorldPin>& outPins)
{
	static std::vector<e2::Collision> _tmp;
	_tmp.clear();

	game()->populateCollisions(coords.offsetCoords(), e2::CollisionType::Component, _tmp, true);
	for (e2::Collision& col : _tmp)
	{
		e2::Entity* ent = col.component->entity();
		e2::RadionEntity* radionEnt = ent->cast<e2::RadionEntity>();
		if (!radionEnt)
			continue;

		e2::RadionEntitySpecification* radionSpec = radionEnt->radionSpecification;
		if (!radionSpec)
			continue;

		uint32_t i = 0;
		for (e2::RadionPin& pin : radionSpec->pins)
		{
			if (pin.type != type)
				continue;

			e2::RadionWorldPin worldPin;
			worldPin.entity = radionEnt;
			worldPin.index = i;
			worldPin.name = pin.name;
			worldPin.worldOffset = radionEnt->getTransform()->getTransformMatrix(e2::TransformSpace::World) * glm::vec4(pin.offset.x, pin.offset.y, pin.offset.z, 1.0f);
			worldPin.type = pin.type;

			outPins.push_back(worldPin);

			i++;
		}
	}
}

void e2::RadionManager::writeForSave(e2::IStream& toBuffer)
{
	toBuffer << (uint64_t)m_discoveredEntities.size();
	for (e2::Name ent : m_discoveredEntities)
	{
		toBuffer << ent;
	}
}

void e2::RadionManager::readForSave(e2::IStream& fromBuffer)
{
	m_discoveredEntities.clear();
	uint64_t numEnts{};
	fromBuffer >> numEnts;
	for (uint64_t i = 0; i < numEnts; i++)
	{
		e2::Name newEnt;
		fromBuffer >> newEnt;
		discoverEntity(newEnt);
	}
}


