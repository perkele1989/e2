#include "game/militaryunit.hpp"



e2::MilitaryUnit::MilitaryUnit(e2::GameContext* ctx, glm::ivec2 const& tile)
	: e2::GameUnit(ctx, tile)

{
	displayName = "Ranger";
	sightRange = 5;
}

e2::MilitaryUnit::~MilitaryUnit()
{

}

void e2::MilitaryUnit::calculateStats()
{

}