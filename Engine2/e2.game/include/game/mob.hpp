
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

	/** @tags(dynamic, arena, arenaSize=256) */
	class MainOperatingBase : public e2::GameStructure
	{
		ObjectDeclaration();

		void setupConfig();

	public:
		MainOperatingBase();
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


	/** @tags(dynamic, arena, arenaSize=256) */
	class Barracks : public e2::GameStructure
	{
		ObjectDeclaration();

		void setupConfig();
	public:
		Barracks();
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

	/** @tags(dynamic, arena, arenaSize=256) */
	class WarFactory : public e2::GameStructure
	{
		ObjectDeclaration();


		void setupConfig();
	public:
		WarFactory();
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

	/** @tags(dynamic, arena, arenaSize=256) */
	class NavalBase : public e2::GameStructure
	{
		ObjectDeclaration();

		void setupConfig();
	public:
		NavalBase();
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

	/** @tags(dynamic, arena, arenaSize=256) */
	class AirBase : public e2::GameStructure
	{
		ObjectDeclaration();

		void setupConfig();
	public:
		AirBase();
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

	/** @tags(dynamic, arena, arenaSize=256) */
	class ForwardOperatingBase : public e2::GameStructure
	{
		ObjectDeclaration();

		void setupConfig();
	public:
		ForwardOperatingBase();
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