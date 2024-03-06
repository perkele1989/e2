
#pragma once 

#include <e2/utils.hpp>
#include <game/gamecontext.hpp>

namespace e2
{
	class MeshProxy;
	class GameUnit : public e2::Object, public e2::GameContext
	{
		ObjectDeclaration();
	public:
		GameUnit(e2::GameContext* ctx, glm::ivec2 const& tile);
		virtual ~GameUnit();

		virtual e2::Game* game() override
		{
			return m_game;
		}

		void spreadVisibility();
		void rollbackVisibility();

		glm::ivec2 tileIndex; // offset coords
		float health{ 100.0f };
		uint32_t sightRange{ 5 };

	protected:
		e2::Game* m_game{};
		e2::MeshProxy* m_proxy{};
	};
}

#include "gameunit.generated.hpp"