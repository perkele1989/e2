
#include "game/entities/pickupentity.hpp"

#include "e2/renderer/meshproxy.hpp"
#include "game/game.hpp"
#include "e2/game/gamesession.hpp"

#include "game/entities/player.hpp"
#include "game/components/staticmeshcomponent.hpp"
#include "e2/managers/audiomanager.hpp"

#include <e2/e2.hpp>
#include <e2/utils.hpp>
#include <e2/transform.hpp>

#include <glm/gtx/quaternion.hpp>
#include <glm/ext/matrix_transform.hpp>

#include <glm/gtx/intersect.hpp>
#include <glm/gtx/vector_angle.hpp>

#include <glm/gtc/noise.hpp>
#include <glm/gtx/spline.hpp>
#include <glm/gtx/easing.hpp>

#include <nlohmann/json.hpp>

#include <fstream>


e2::PickupSpecification::PickupSpecification()
	: e2::EntitySpecification()
{
	entityType = e2::Type::fromName("e2::PickupEntity");
}

e2::PickupSpecification::~PickupSpecification()
{

}

void e2::PickupSpecification::populate(e2::GameContext* ctx, nlohmann::json& obj)
{
	e2::EntitySpecification::populate(ctx, obj);

	if (obj.contains("collision"))
		collision.populate(obj.at("collision"), assets);

	if (obj.contains("pickupMesh"))
		pickupMesh.populate(obj.at("pickupMesh"), assets);

	if (obj.contains("pickupItem"))
	{
		pickupItem = obj.at("pickupItem").template get<std::string>();
	}

}

void e2::PickupSpecification::finalize()
{
	pickupMesh.finalize(game);
}

e2::PickupEntity::PickupEntity()
	: e2::Entity()
{
}

e2::PickupEntity::~PickupEntity()
{
	if (m_mesh)
		e2::destroy(m_mesh);

	if (m_collision)
		e2::destroy(m_collision);

}

void e2::PickupEntity::postConstruct(e2::GameContext* ctx, e2::EntitySpecification* spec, glm::vec3 const& worldPosition, glm::quat const& worldRotation)
{
	e2::Entity::postConstruct(ctx, spec, worldPosition, worldRotation);
	m_pickupSpecification = getSpecificationAs<e2::PickupSpecification>();
	m_mesh = e2::create<e2::StaticMeshComponent>(&m_pickupSpecification->pickupMesh, this);
	m_collision = e2::create<e2::CollisionComponent>(&m_pickupSpecification->collision, this);
}

void e2::PickupEntity::updateAnimation(double seconds)
{
	e2::Entity::updateAnimation(seconds);

	m_mesh->applyTransform();
}

void e2::PickupEntity::update(double seconds)
{
	e2::Entity::update(seconds);

	m_collision->invalidate();
}

void e2::PickupEntity::updateVisibility()
{
	e2::Entity::updateVisibility();
	m_mesh->updateVisibility();
}

void e2::PickupEntity::onInteract(e2::Entity* interactor)
{
	if (pendingDestroy)
		return;

	game()->playerState().give(m_pickupSpecification->pickupItem);
	game()->queueDestroyEntity(this);
}
