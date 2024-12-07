
#pragma once 

#include <e2/utils.hpp>
#include "game/game.hpp"
#include "game/components/skeletalmeshcomponent.hpp"
#include "game/components/audiocomponent.hpp"
#include "game/components/physicscomponent.hpp"
#include "game/components/fogcomponent.hpp"


namespace e2
{

	/** @tags(dynamic) */
	class AmberlingSpecification : public e2::EntitySpecification
	{
		ObjectDeclaration();
	public:
		AmberlingSpecification();
		virtual ~AmberlingSpecification();
		virtual void populate(e2::GameContext* ctx, nlohmann::json& obj) override;
		virtual void finalize() override;

		e2::SkeletalMeshSpecification mesh;

		e2::CollisionSpecification collision;
		e2::MovementSpecification movement;

	};

	/**
	 * A game character, i.e. a freely movable character (not strictly tied to the hexgrid, but navigates through it freely with collisions)
	 * @tags(dynamic, arena, arenaSize=1024)
	 */
	class AmberlingEntity : public e2::Entity, public e2::ITriggerListener
	{
		ObjectDeclaration();
	public:
		AmberlingEntity();
		virtual ~AmberlingEntity();

		virtual void postConstruct(e2::GameContext* ctx, e2::EntitySpecification* spec, glm::vec3 const& worldPosition, glm::quat const& worldRotation) override;

		virtual void writeForSave(e2::IStream& toBuffer) override;
		virtual void readForSave(e2::IStream& fromBuffer) override;

		virtual void updateAnimation(double seconds) override;

		virtual void update(double seconds) override;

		virtual void onTrigger(e2::Name action, e2::Name trigger) override;

		virtual void updateVisibility() override;

		virtual void onMeleeDamage(e2::Entity* instigator, float dmg) override;

		virtual bool canBeDestroyed() override;


		virtual bool interactable() override { return true; }

		virtual void onInteract(e2::Entity* interactor) override;


	protected:

		e2::AmberlingSpecification* m_amberlingSpecification{};
		float m_spawnCooldown{ 5.0f };
		float m_health{ 30.0f };
		bool m_interacting{ false };

		e2::CollisionComponent* m_collision{};
		e2::MovementComponent* m_movement{};

		e2::SkeletalMeshComponent* m_mesh{};
		glm::quat m_targetRotation{ glm::identity<glm::quat>() };

		glm::vec2 m_inputVector{};

		glm::vec2 m_hitDirection{};

		bool m_jumping{};
		float m_jumpFactor{};
		float m_timeSinceJump{};

		glm::vec2 m_lastPosition{};

		std::vector<e2::Collision> m_collisionCache;
	};

}

#include "amberling.generated.hpp"