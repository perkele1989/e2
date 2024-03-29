
#pragma once 

#include "gameunit.hpp"

namespace e2
{

	struct BuildAction
	{
		std::string displayName;
		e2::EntityType buildType{};
		int32_t buildTurnsLeft{};
		int32_t buildTurns{};
	};

	/** @tags(arena, arenaSize=256) */
	class MainOperatingBase : public e2::GameStructure
	{
		ObjectDeclaration();
	public:
		MainOperatingBase(e2::GameContext* gameCtx, glm::ivec2 const& tile, uint8_t empire);
		virtual ~MainOperatingBase();
		virtual void drawUI(e2::UIContext* ctx) override;


		virtual void onTurnEnd() override;
		virtual void onTurnStart() override;

	protected:

		std::string m_buildMessage;
		BuildAction* m_currentlyBuilding{};

		BuildAction m_buildActionEngineer;
	};


	/** @tags(arena, arenaSize=256) */
	class Barracks : public e2::GameStructure
	{
		ObjectDeclaration();
	public:
		Barracks(e2::GameContext* gameCtx, glm::ivec2 const& tile, uint8_t empire);
		virtual ~Barracks();
		virtual void drawUI(e2::UIContext* ctx) override;


		virtual void onTurnEnd() override;
		virtual void onTurnStart() override;

	protected:

		std::string m_buildMessage;
		BuildAction* m_currentlyBuilding{};

		BuildAction m_buildActionGrunt;
	};

	/** @tags(arena, arenaSize=256) */
	class WarFactory : public e2::GameStructure
	{
		ObjectDeclaration();
	public:
		WarFactory(e2::GameContext* gameCtx, glm::ivec2 const& tile, uint8_t empire);
		virtual ~WarFactory();
		virtual void drawUI(e2::UIContext* ctx) override;


		virtual void onTurnEnd() override;
		virtual void onTurnStart() override;

	protected:

		std::string m_buildMessage;
		BuildAction* m_currentlyBuilding{};

		BuildAction m_buildActionTank;
	};

	/** @tags(arena, arenaSize=256) */
	class NavalBase : public e2::GameStructure
	{
		ObjectDeclaration();
	public:
		NavalBase(e2::GameContext* gameCtx, glm::ivec2 const& tile, uint8_t empire);
		virtual ~NavalBase();
		virtual void drawUI(e2::UIContext* ctx) override;


		virtual void onTurnEnd() override;
		virtual void onTurnStart() override;

	protected:

		std::string m_buildMessage;
		BuildAction* m_currentlyBuilding{};

		BuildAction m_buildActionCombatBoat;
	};

	/** @tags(arena, arenaSize=256) */
	class AirBase : public e2::GameStructure
	{
		ObjectDeclaration();
	public:
		AirBase(e2::GameContext* gameCtx, glm::ivec2 const& tile, uint8_t empire);
		virtual ~AirBase();
		virtual void drawUI(e2::UIContext* ctx) override;


		virtual void onTurnEnd() override;
		virtual void onTurnStart() override;

	protected:

		std::string m_buildMessage;
		BuildAction* m_currentlyBuilding{};

		BuildAction m_buildActionJetFighter;
	};

	/** @tags(arena, arenaSize=256) */
	class ForwardOperatingBase : public e2::GameStructure
	{
		ObjectDeclaration();
	public:
		ForwardOperatingBase(e2::GameContext* gameCtx, glm::ivec2 const& tile, uint8_t empire);
		virtual ~ForwardOperatingBase();
		virtual void drawUI(e2::UIContext* ctx) override;


		virtual void onTurnEnd() override;
		virtual void onTurnStart() override;

	protected:

		std::string m_buildMessage;
		BuildAction* m_currentlyBuilding{};

		BuildAction m_buildActionEngineer;
	};

}

#include "mob.generated.hpp"