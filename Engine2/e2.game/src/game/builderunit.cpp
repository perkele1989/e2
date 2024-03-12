
#include "game/builderunit.hpp"
#include "game/game.hpp"


e2::Engineer::Engineer(e2::GameContext* ctx, glm::ivec2 const& tile, uint8_t empire)
	: e2::GameUnit(ctx, tile, empire)

{
	displayName = "Engineer";
	sightRange = 5;
	moveRange = 8;

	unitType = e2::GameUnitType::Engineer;
	m_mesh = game()->getUnitMesh(unitType);
	buildProxy();
}

e2::Engineer::~Engineer()
{

}