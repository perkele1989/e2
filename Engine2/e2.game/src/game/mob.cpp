
#include "game/mob.hpp"
#include "game/game.hpp"

e2::MainOperatingBase::MainOperatingBase(e2::GameContext* gameCtx, glm::ivec2 const& tile, uint8_t empire)
	: e2::GameStructure(gameCtx, tile, empire)
{
	displayName = "Main Operating Base";
	sightRange = 8;
	
	structureType = e2::GameStructureType::MainOperatingBase;
	m_mesh = game()->getStructureMesh(structureType);
	buildProxy();
}

e2::MainOperatingBase::~MainOperatingBase()
{

}

