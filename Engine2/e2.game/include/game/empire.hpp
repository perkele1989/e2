
#pragma once 

#include <e2/utils.hpp>
#include "game/gamecontext.hpp"

#include <unordered_set>

namespace e2
{
	class Game;
	class GameUnit;
	class GameStructure;

	class GameEmpire : public e2::Object, public e2::GameContext
	{
		ObjectDeclaration();
	public:
		GameEmpire();
		GameEmpire(e2::GameContext* ctx, uint8_t id);
		virtual ~GameEmpire();

		uint8_t id;

		virtual e2::Game* game() override;

		std::unordered_set<e2::GameUnit*> units;
		std::unordered_set<e2::GameStructure*> structures;

	protected:
		e2::Game* m_game{};

		
	};

}

#include "empire.generated.hpp"