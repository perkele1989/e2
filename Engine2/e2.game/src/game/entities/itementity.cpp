
#include "game/entities/itementity.hpp"

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


e2::ItemSpecification::ItemSpecification()
	: e2::EntitySpecification()
{
	entityType = e2::Type::fromName("e2::ItemEntity");
}

e2::ItemSpecification::~ItemSpecification()
{
	if (wieldHandler)
		e2::destroy(wieldHandler);

	if (useHandler)
		e2::destroy(useHandler);
}

void e2::ItemSpecification::populate(e2::GameContext* ctx, nlohmann::json& obj)
{
	e2::EntitySpecification::populate(ctx, obj);
	if (obj.contains("stackable"))
	{
		stackable = obj.at("stackable").template get<bool>();
	}

	if (obj.contains("stackSize"))
	{
		stackSize = obj.at("stackSize").template get<uint32_t>();
	}


	if (obj.contains("equippable"))
	{
		equippable = obj.at("equippable").template get<bool>();
	}

	if (obj.contains("wieldable"))
	{
		wieldable = obj.at("wieldable").template get<bool>();
	}

	if (obj.contains("usable"))
	{
		usable = obj.at("usable").template get<bool>();
	}

	if (obj.contains("wieldHandler"))
	{
		std::string wieldTypeName = obj.at("wieldHandler").template get<std::string>();
		wieldHandlerType = e2::Type::fromName(wieldTypeName);
		if (!wieldHandlerType)
		{
			LogError("unrecognized type for wieldhandler");
			return;
		}

		wieldHandler = wieldHandlerType->create()->cast<e2::WieldHandler>();
		if (!wieldHandler)
		{
			LogError("failed to instance wieldhandler, make sure its not purevirtual");
			return;
		}

		if (obj.contains("wieldHandlerData"))
		{
			wieldHandler->populate(ctx, obj.at("wieldHandlerData"), assets);
		}
	}

	if (obj.contains("useHandler"))
	{
		useHandlerType = e2::Type::fromName(obj.at("useHandler").template get<std::string>());
		if (!useHandlerType)
		{
			LogError("unrecognized type for usehandler");
			return;
		}

		useHandler = useHandlerType->create()->cast<e2::UseHandler>();
		if (!useHandler)
		{
			LogError("failed to instance usehandler, make sure its not purevirtual");
			return;
		}

		if (obj.contains("useHandlerData"))
		{
			useHandler->populate(ctx, obj.at("useHandlerData"));
		}
	}


	if (obj.contains("equipHandler"))
	{
		equipHandlerType = e2::Type::fromName(obj.at("equipHandler").template get<std::string>());
		if (!equipHandlerType)
		{
			LogError("unrecognized type for equiphandler");
			return;
		}

		equipHandler = equipHandlerType->create()->cast<e2::EquipHandler>();
		if (!equipHandler)
		{
			LogError("failed to instance equiphandler, make sure its not purevirtual");
			return;
		}

		if (obj.contains("equipHandlerData"))
		{
			equipHandler->populate(ctx, obj.at("equipHandlerData"));
		}
	}

	if (obj.contains("equipMesh"))
		equipMesh.populate(obj.at("equipMesh"), assets);

	if (obj.contains("dropMesh"))
	{
		dropMesh.populate(obj.at("dropMesh"), assets);
	}

	if (obj.contains("iconSprite"))
	{
		iconSprite = obj.at("iconSprite").template get<std::string>();
	}

	collision.radius = 0.1;

	if (obj.contains("movement"))
	{
		movement.populate(obj.at("movement"), assets);
	}

}

void e2::ItemSpecification::finalize()
{
	dropMesh.finalize(game);
	equipMesh.finalize(game);

	if (equipHandler)
		equipHandler->finalize(game);

	if (useHandler)
		useHandler->finalize(game);

	if (wieldHandler)
		wieldHandler->finalize(game);
}

e2::ItemEntity::ItemEntity()
	: e2::Entity()
{
}

e2::ItemEntity::~ItemEntity()
{
	if (m_mesh)
		e2::destroy(m_mesh);

	if (m_collision)
		e2::destroy(m_collision);

	if (m_movement)
		e2::destroy(m_movement);
}

void e2::ItemEntity::postConstruct(e2::GameContext* ctx, e2::EntitySpecification* spec, glm::vec3 const& worldPosition, glm::quat const& worldRotation)
{
	e2::Entity::postConstruct(ctx, spec, worldPosition, worldRotation);
	m_itemSpecification = getSpecificationAs<e2::ItemSpecification>();
	m_mesh = e2::create<e2::StaticMeshComponent>(&m_itemSpecification->dropMesh, this);
	m_collision = e2::create<e2::CollisionComponent>(&m_itemSpecification->collision, this);
	m_movement = e2::create<e2::MovementComponent>(&m_itemSpecification->movement, this);
}

void e2::ItemEntity::writeForSave(e2::IStream& toBuffer)
{
	e2::Entity::writeForSave(toBuffer);
	toBuffer << m_time << m_rotation;
}

void e2::ItemEntity::readForSave(e2::IStream& fromBuffer)
{
	e2::Entity::readForSave(fromBuffer);
	fromBuffer >> m_time >> m_rotation;
}

void e2::ItemEntity::updateAnimation(double seconds)
{
	e2::Entity::updateAnimation(seconds);

	constexpr float rotationsPerSecond = 0.25f;
	constexpr double periodsPerSecond = 0.24;
	constexpr double maxHeight = 0.10;

	double sineWave = glm::sin(glm::radians(m_time * 360.0f * periodsPerSecond));
	double sineWaveUnit = sineWave * 0.5 + 0.5;
	double height = sineWaveUnit * -maxHeight;
	m_mesh->setHeightOffset(height);

	m_rotation += seconds * 360.0f * rotationsPerSecond;

	getTransform()->setRotation(glm::rotate(glm::identity<glm::quat>(), glm::radians(m_rotation), e2::worldUpf()), e2::TransformSpace::World);

	m_mesh->applyTransform();
}

void e2::ItemEntity::update(double seconds)
{
	e2::Entity::update(seconds);


	m_time += seconds;

	if (m_coolDown >= 0.0f)
	{
		m_coolDown -= seconds;
	}

	m_collisionCache.clear();
	game()->populateCollisions(hex().offsetCoords(), e2::CollisionType::Component | e2::CollisionType::Tree | e2::CollisionType::Mountain, m_collisionCache);

	m_movement->resolve(0.1f, e2::CollisionType::Component | e2::CollisionType::Tree | e2::CollisionType::Mountain, m_collisionCache);
	m_collision->invalidate();
}

void e2::ItemEntity::updateVisibility()
{
	e2::Entity::updateVisibility();
	m_mesh->updateVisibility();
}
