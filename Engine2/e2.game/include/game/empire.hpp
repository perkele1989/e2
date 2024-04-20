
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
	class EmpireAI;

	/** @tags(arena, arenaSize=e2::maxNumEmpires) */
	class GameEmpire : public e2::Object, public e2::GameContext, public e2::Data
	{
		ObjectDeclaration();
	public:
		GameEmpire();
		GameEmpire(e2::GameContext* ctx, EmpireId id);
		virtual ~GameEmpire();

		EmpireId id;

		virtual e2::Game* game() override;

		virtual void write(Buffer& destination) const override;
		virtual bool read(Buffer& source) override;

		std::unordered_set<e2::GameEntity*> entities;

		// optional
		EmpireAI* ai{};

		GameResources resources;
	protected:
		e2::Game* m_game{};

		
	};

	class GameEntity;
	class GameUnit; 
	class GameStructure;
	class EmpireAI;

	using EntityActionTickFunc = void(*)(e2::EmpireAI*, e2::GameEntity*);

	struct EntityAction
	{
		EntityActionTickFunc tick{};
	};

	class EmpireAI : public e2::Object, public e2::GameContext
	{
		ObjectDeclaration();
	public:
		EmpireAI();
		EmpireAI(e2::GameContext* ctx, EmpireId empireId);
		virtual ~EmpireAI();

		virtual void grugBrainWakeUp();
		virtual void grugBrainTick(double seconds);
		virtual void grugBrainGoSleep();

		virtual e2::Game* game() override;
		EmpireId id;
		e2::GameEmpire* empire{};

		e2::GameEntity* currentEntity{};
		std::unordered_set<e2::GameEntity*> turnEntities;

		void collectEntities();

		EntityAction getActionForEntity(e2::GameEntity* entity);

	protected:
		e2::Game* m_game{};
	};

	class NomadAI : public e2::EmpireAI
	{
		ObjectDeclaration();
	public:
		NomadAI();
		NomadAI(e2::GameContext* ctx, EmpireId empireId);
		virtual ~NomadAI();
		virtual void grugBrainWakeUp() override;
		virtual void grugBrainTick(double seconds) override;
		virtual void grugBrainGoSleep() override;
	};

	class CommanderAI : public e2::EmpireAI
	{
		ObjectDeclaration();
	public:
		CommanderAI();
		CommanderAI(e2::GameContext* ctx, EmpireId empireId);
		virtual ~CommanderAI(); 

		virtual void grugBrainTick(double seconds) override;
	};

}

#include "empire.generated.hpp"