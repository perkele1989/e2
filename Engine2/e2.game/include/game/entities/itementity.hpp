
#pragma once 


#include <e2/utils.hpp>
#include <e2/assets/mesh.hpp>
#include <e2/ui/uicontext.hpp>
#include <e2/transform.hpp>
#include <e2/renderer/meshproxy.hpp>

#include <game/gamecontext.hpp>
#include "game/shared.hpp"
#include "game/entity.hpp"
#include "game/components/staticmeshcomponent.hpp"
#include "game/components/physicscomponent.hpp"
#include "game/components/audiocomponent.hpp"
#include "game/resources.hpp"

#include <nlohmann/json.hpp>

#include <string>


namespace e2
{

	class WieldHandler;
	class UseHandler;
	class EquipHandler;

	class ItemSpecification : public e2::EntitySpecification
	{
		ObjectDeclaration();
	public:
		ItemSpecification();
		virtual ~ItemSpecification();

		virtual void populate(e2::GameContext* ctx, nlohmann::json& obj) override;
		virtual void finalize() override;

		bool stackable{};
		uint32_t stackSize{ 1 };

		bool wieldable{};
		e2::Type* wieldHandlerType{};
		e2::WieldHandler* wieldHandler{}; // owning instance

		bool usable{};
		e2::Type* useHandlerType{};
		e2::UseHandler* useHandler{}; // owning instance

		bool equippable{};
		e2::EquipSlot equipSlot{ e2::EquipSlot::Shield };
		e2::Type* equipHandlerType{};
		e2::EquipHandler* equipHandler{};


		e2::Name iconSprite;

		StaticMeshSpecification equipMesh;
		StaticMeshSpecification dropMesh;
		CollisionSpecification collision;
		MovementSpecification movement;

	};


	/** @tags(dynamic) */
	class ItemEntity : public e2::Entity
	{
		ObjectDeclaration();
	public:
		ItemEntity();
		virtual ~ItemEntity();

		virtual void postConstruct(e2::GameContext* ctx, e2::EntitySpecification* spec, glm::vec3 const& worldPosition, glm::quat const& worldRotation) override;

		virtual void writeForSave(e2::IStream& toBuffer) override;
		virtual void readForSave(e2::IStream& fromBuffer) override;

		virtual void updateAnimation(double seconds) override;

		virtual void update(double seconds) override;

		virtual void updateVisibility() override;

		inline double getLifetime() {
			return m_time;
		}

		inline void setLifetime(double newTime)
		{
			m_time = newTime;
		}

	protected:
		e2::ItemSpecification* m_itemSpecification{};
		e2::StaticMeshComponent* m_mesh{};
		e2::CollisionComponent* m_collision{};
		e2::MovementComponent* m_movement{};

		double m_time{};
		float m_rotation{};

		std::vector<e2::Collision> m_collisionCache;
	};

}

#include "itementity.generated.hpp"