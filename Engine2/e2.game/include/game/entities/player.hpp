#pragma once 

#include <e2/utils.hpp>
#include "game/game.hpp"
#include "game/components/skeletalmeshcomponent.hpp"
#include "game/components/audiocomponent.hpp"
#include "game/components/physicscomponent.hpp"
#include "game/components/fogcomponent.hpp"

namespace e2
{
	struct RadialCompare
	{
		bool operator() (e2::Entity* a, e2::Entity* b) const;
	};

	/** @tags(dynamic) */
	class PlayerSpecification : public e2::EntitySpecification
	{
		ObjectDeclaration();
	public:
		PlayerSpecification();
		virtual ~PlayerSpecification();

		virtual void populate(e2::GameContext* ctx, nlohmann::json& obj) override;
		virtual void finalize() override;
		/** The move speed */
		float moveSpeed{ 1.0f };

		e2::SkeletalMeshSpecification mesh;
		e2::StaticMeshSpecification boatMesh;
		e2::RandomAudioSpecification footstepGrass;
		e2::RandomAudioSpecification footstepDesert;
		e2::RandomAudioSpecification footstepSnow;
		e2::CollisionSpecification collision;
		e2::MovementSpecification movement;
	};

	/**
	 * A game character, i.e. a freely movable character (not strictly tied to the hexgrid, but navigates through it freely with collisions)
	 * @tags(dynamic, arena, arenaSize=32)
	 */
	class PlayerEntity : public e2::Entity, public e2::ITriggerListener
	{
		ObjectDeclaration();
	public:
		PlayerEntity();
		virtual ~PlayerEntity();

		virtual void postConstruct(e2::GameContext* ctx, e2::EntitySpecification* spec, glm::vec3 const& worldPosition, glm::quat const& worldRotation) override;

		virtual void writeForSave(e2::IStream& toBuffer) override;
		virtual void readForSave(e2::IStream& fromBuffer) override;

		virtual void updateAnimation(double seconds) override;

		

		virtual void update(double seconds) override;

		void updateLand(double seconds);
		void updateWater(double seconds);

		void setInputVector(glm::vec2 const& inputVector);

		inline bool isCaptain() {
			return m_captain;
		}

		virtual void onTrigger(e2::Name action, e2::Name trigger) override;

		virtual  void onHitEntity(e2::Entity* otherEntity) override;

		virtual void updateVisibility() override;

		inline bool aiming() {
			return m_aiming;
		}


		inline e2::SkeletalMeshComponent* getMesh()
		{
			return m_mesh;
		}
		inline e2::CollisionComponent* getCollision()
		{
			return m_collision;
		}
		inline e2::MovementComponent* getMovement()
		{
			return m_movement;
		}
		inline std::vector<e2::Collision>& getCollisionCache()
		{
			return m_collisionCache;
		}

	protected:


		e2::PlayerSpecification* m_playerSpecification{};

		e2::SkeletalMeshComponent* m_mesh{};
		e2::StaticMeshComponent* m_boatMesh{};

		bool m_captain{};
		float m_boatVelocity{0.0f};

		glm::quat m_currentRotation{ glm::identity<glm::quat>() };
		glm::quat m_targetRotation{ glm::identity<glm::quat>()};

		glm::quat m_tilt{ glm::identity<glm::quat>() };

		e2::RandomAudioComponent* m_footstepGrass{};
		e2::RandomAudioComponent* m_footstepDesert{};
		e2::RandomAudioComponent* m_footstepSnow{};

		e2::CollisionComponent* m_collision{};
		e2::MovementComponent* m_movement{};

		bool m_actionBusy{};

		bool m_aiming{};

		glm::vec2 m_inputVector{};
		glm::vec2 m_lastPosition{};

		e2::FogComponent m_fog;

		e2::Entity* m_interactable{};
		std::vector<e2::Collision> m_collisionCache;
	};

}


#include "player.generated.hpp"