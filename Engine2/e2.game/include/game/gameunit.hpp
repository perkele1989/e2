
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

		void setMeshTransform(glm::vec3 const& pos, float angle);

		void spreadVisibility();
		void rollbackVisibility();

		std::string displayName{ "Unit" };

		float health{ 100.0f };
		uint32_t sightRange{ 5 };
		uint32_t moveRange{ 3 };

		glm::ivec2 tileIndex; // offset coords

		virtual void updateAnimation(double seconds);

	protected:
		e2::Game* m_game{};
		e2::MeshProxy* m_proxy{};
		
		glm::quat m_rotation;
		glm::quat m_targetRotation;
		glm::vec3 m_position;
	};
}

#include "gameunit.generated.hpp"