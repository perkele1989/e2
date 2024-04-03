
#pragma once 

#include <e2/utils.hpp>
#include "game/gamecontext.hpp"
#include "game/shared.hpp"
#include "game/resources.hpp"

#include <unordered_set>

namespace e2
{
	class Game;
	class GameUnit;
	class GameStructure;
	class CommanderAI;

	/** @tags(arena, arenaSize=e2::maxNumEmpires) */
	class GameEmpire : public e2::Object, public e2::GameContext
	{
		ObjectDeclaration();
	public:
		GameEmpire();
		GameEmpire(e2::GameContext* ctx, EmpireId id);
		virtual ~GameEmpire();

		EmpireId id;

		virtual e2::Game* game() override;

		std::unordered_set<e2::GameUnit*> units;
		std::unordered_set<e2::GameStructure*> structures;

		CommanderAI* ai{};
		GameResources resources;
	protected:
		e2::Game* m_game{};

		
	};

	class GameEntity;
	class GameUnit; 
	class GameStructure;

	class CommanderAI : public e2::Object, public e2::GameContext
	{
		ObjectDeclaration();
	public:
		CommanderAI();
		CommanderAI(e2::GameContext* ctx, EmpireId empireId);
		virtual ~CommanderAI(); 

		virtual void grugBrainTick();

		virtual e2::Game* game() override;
		EmpireId id;
		e2::GameEmpire* empire{};

		e2::GameEntity* currentEntity{};
		std::unordered_set<e2::GameEntity*> turnEntities;

	protected:
		e2::Game* m_game{};
	};

}

#include "empire.generated.hpp"