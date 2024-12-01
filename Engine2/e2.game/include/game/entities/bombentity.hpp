
#pragma once 


#include <e2/utils.hpp>
#include "game/game.hpp"
#include "game/components/skeletalmeshcomponent.hpp"
#include "game/components/audiocomponent.hpp"
#include "game/components/physicscomponent.hpp"
#include "game/components/fogcomponent.hpp"

#include "game/entities/throwableentity.hpp"

namespace e2
{


	/** @tags(dynamic) */
	class BombSpecification : public e2::EntitySpecification
	{
		ObjectDeclaration();
	public:
		BombSpecification();
		virtual ~BombSpecification();
		virtual void populate(e2::GameContext* ctx, nlohmann::json& obj) override;
		virtual void finalize() override;

		e2::StaticMeshSpecification mesh;
		e2::CollisionSpecification collision;
		e2::MovementSpecification movement;


		bool isRedBomb{};

	};


	/**
	 * @tags(dynamic, arena, arenaSize=1024)
	 */
	class BombEntity : public e2::ThrowableEntity
	{
		ObjectDeclaration();
	public:
		BombEntity();
		virtual ~BombEntity();

		virtual void postConstruct(e2::GameContext* ctx, e2::EntitySpecification* spec, glm::vec3 const& worldPosition, glm::quat const& worldRotation) override;

		virtual void writeForSave(e2::IStream& toBuffer) override;
		virtual void readForSave(e2::IStream& fromBuffer) override;

		virtual void updateAnimation(double seconds) override;

		virtual void update(double seconds) override;

		virtual void updateVisibility() override;


		virtual void onMeleeDamage(e2::Entity* instigator, float dmg) override;

		virtual void setVelocity(glm::vec3 newVelocity) override;

	protected:

		e2::BombSpecification* m_bombSpecification{};

		glm::vec3 m_velocity{};

		e2::CollisionComponent* m_collision{};
		e2::MovementComponent* m_movement{};

		e2::StaticMeshComponent* m_mesh{};
		glm::vec2 m_lastPosition{};

		float m_life{ 3.0f };
		bool m_dead = false;
		float m_killTimer{ 0.1f };

		std::vector<e2::Collision> m_collisionCache;
	};

}

#include "bombentity.generated.hpp"