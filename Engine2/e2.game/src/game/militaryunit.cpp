
#include "game/militaryunit.hpp"
#include "game/game.hpp"


e2::MilitaryUnit::MilitaryUnit(e2::GameContext* ctx, glm::ivec2 const& tile, uint8_t empire)
	: e2::GameUnit(ctx, tile, empire)

{
	displayName = "Ranger";
	sightRange = 5;

	unitType = e2::GameUnitType::Ranger;
	m_mesh = game()->getUnitMesh(unitType);
	buildProxy();
}

e2::MilitaryUnit::~MilitaryUnit()
{

}

void e2::MilitaryUnit::calculateStats()
{

}