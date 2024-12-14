
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
	/** @tags(dynamic) */
	class PickupSpecification : public e2::EntitySpecification
	{
		ObjectDeclaration();
	public:
		PickupSpecification();
		virtual ~PickupSpecification();

		virtual void populate(e2::GameContext* ctx, nlohmann::json& obj) override;
		virtual void finalize() override;

		e2::Name pickupItem;
		StaticMeshSpecification pickupMesh;
		CollisionSpecification collision;
		
	};


	/** @tags(dynamic) */
	class PickupEntity : public e2::Entity
	{
		ObjectDeclaration();
	public:
		PickupEntity();
		virtual ~PickupEntity();

		virtual void postConstruct(e2::GameContext* ctx, e2::EntitySpecification* spec, glm::vec3 const& worldPosition, glm::quat const& worldRotation) override;

		virtual void updateAnimation(double seconds) override;

		virtual void update(double seconds) override;

		virtual void updateVisibility() override;

		virtual bool interactable() override { return true; }

		virtual void onInteract(e2::Entity* interactor);

		virtual std::string interactText() override { return "Pick up"; }

	protected:
		e2::PickupSpecification* m_pickupSpecification{};
		e2::StaticMeshComponent* m_mesh{};
		e2::CollisionComponent* m_collision{};
	};

}

#include "pickupentity.generated.hpp"