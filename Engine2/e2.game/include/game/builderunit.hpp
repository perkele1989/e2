
#pragma once

#include "game/gameunit.hpp"

namespace e2
{



	/** @tags(arena, arenaSize=4096)*/
	class Engineer : public e2::GameUnit
	{
		ObjectDeclaration();
	public:
		Engineer(e2::GameContext* ctx, glm::ivec2 const& tile, uint8_t empire);
		virtual ~Engineer();

		virtual void drawUI(e2::UIContext* ctx) override;
	};
}

#include "builderunit.generated.hpp"