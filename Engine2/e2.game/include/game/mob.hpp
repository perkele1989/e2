
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

}

#include "mob.generated.hpp"