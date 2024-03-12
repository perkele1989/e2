
#pragma once 

#include "gameunit.hpp"

namespace e2
{

	/** @tags(arena, arenaSize=256) */
	class MainOperatingBase : public e2::GameStructure
	{
		ObjectDeclaration();
	public:
		MainOperatingBase(e2::GameContext* gameCtx, glm::ivec2 const& tile, uint8_t empire);
		virtual ~MainOperatingBase();

	protected:
	};

}

#include "mob.generated.hpp"