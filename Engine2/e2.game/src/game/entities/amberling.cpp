

#include "game/entities/amberling.hpp"

#include "game/entities/player.hpp"
#include "game/entities/itementity.hpp"
#include "game/game.hpp"

#include <e2/game/gamesession.hpp>
#include <e2/e2.hpp>

#include <e2/utils.hpp>


e2::AmberlingSpecification::AmberlingSpecification()
	: e2::EntitySpecification()
{
	entityType = e2::Type::fromName("e2::AmberlingEntity");
}

e2::AmberlingSpecification::~AmberlingSpecification()
{
}

void e2::AmberlingSpecification::populate(e2::GameContext* ctx, nlohmann::json& obj)
{
	e2::EntitySpecification::populate(ctx, obj);

	assets.insert("S_Teardrop_Jump.e2a");
	assets.insert("S_Teardrop_Land.e2a");
	assets.insert("S_Teardrop_Hit.e2a");
	assets.insert("S_Teardrop_Die.e2a");

	if (obj.contains("mesh"))
		mesh.populate(obj.at("mesh"), assets);

	if (obj.contains("collision"))
		collision.populate(obj.at("collision"), assets);

	if (obj.contains("movement"))
		movement.populate(obj.at("movement"), assets);
}

void e2::AmberlingSpecification::finalize()
{
	mesh.finalize(game);
	collision.finalize(game);

}

e2::AmberlingEntity::AmberlingEntity()
	: e2::Entity()
{
	m_targetRotation = glm::angleAxis(0.0f, e2::worldUpf());
}

e2::AmberlingEntity::~AmberlingEntity()
{
	if (m_mesh)
		e2::destroy(m_mesh);

	if (m_collision)
		e2::destroy(m_collision);

	if (m_movement)
		e2::destroy(m_movement);

}

void e2::AmberlingEntity::postConstruct(e2::GameContext* ctx, e2::EntitySpecification* spec, glm::vec3 const& worldPosition, glm::quat const& worldRotation)
{
	e2::Entity::postConstruct(ctx, spec, worldPosition, worldRotation);
	m_amberlingSpecification = m_specification->cast<e2::AmberlingSpecification>();

	m_mesh = e2::create<e2::SkeletalMeshComponent>(&m_amberlingSpecification->mesh, this);
	m_mesh->setTriggerListener(this);


	m_collision = e2::create<e2::CollisionComponent>(&m_amberlingSpecification->collision, this);
	m_movement = e2::create<e2::MovementComponent>(&m_amberlingSpecification->movement, this);

	m_timeSinceJump = e2::randomFloat(0.0f, 2.25f);
}

void e2::AmberlingEntity::writeForSave(e2::IStream& toBuffer)
{
}

void e2::AmberlingEntity::readForSave(e2::IStream& fromBuffer)
{
}

void e2::AmberlingEntity::updateAnimation(double seconds)
{
	glm::quat currentRotation = m_transform->getRotation(e2::TransformSpace::World);
	currentRotation = glm::slerp(currentRotation, m_targetRotation, glm::clamp(float(seconds) * 10.0f, 0.01f, 1.0f));
	m_transform->setRotation(currentRotation, e2::TransformSpace::World);

	m_mesh->updateAnimation(seconds);
}

void e2::AmberlingEntity::update(double seconds)
{
	if(m_spawnCooldown >= 0.0f)
		m_spawnCooldown -= seconds;

	e2::Entity* player = game()->playerEntity();
	if (!player)
		return;

	glm::vec2 playerCoords = player->planarCoords();
	if (m_spawnCooldown <= 0.0f && glm::distance(planarCoords(), playerCoords) > 15.0f)
	{
		game()->queueDestroyEntity(this);
		return;
	}

	m_collisionCache.clear();
	game()->populateCollisions(hex().offsetCoords(), e2::CollisionType::All, m_collisionCache);

	
	glm::vec2 thisCoords = planarCoords();

	constexpr float jumpInterval = 3.0f;
	constexpr float jumpTime = 0.5f;
	constexpr float rotateTime = 2.3f;
	constexpr float jumpHeight = 0.3f;

	if (m_interacting)
	{
		float angle = e2::radiansBetween(glm::vec3(playerCoords.x, 0.0f, playerCoords.y), glm::vec3(thisCoords.x, 0.0f, thisCoords.y));
		m_targetRotation = glm::angleAxis(angle, glm::vec3(e2::worldUp()));
		m_mesh->setPose("idle");

		if (glm::distance(playerCoords, thisCoords) >= 0.5f)
			m_interacting = false;
	}
	else if (m_mesh->isActionPlaying("die") || m_health <= 0.0f)
	{

	}
	else if (m_mesh->isActionPlaying("hit"))
	{
		m_movement->move(m_collision->radius(), m_hitDirection, float(seconds) * 1.0f, e2::CollisionType::All, m_collisionCache);
		float angle = e2::radiansBetween(glm::vec3(thisCoords.x, 0.0f, thisCoords.y) - glm::vec3(m_hitDirection.x, 0.0f, m_hitDirection.y), glm::vec3(thisCoords.x, 0.0f, thisCoords.y));
		m_targetRotation = glm::angleAxis(angle, glm::vec3(e2::worldUp()));
		m_jumping = false;
		m_timeSinceJump = 0.0f;
	}
	else if (m_jumping)
	{
		m_mesh->setPose("air");

		m_jumpFactor += float(seconds);

		float jumpCoefficient = glm::clamp(m_jumpFactor / jumpTime, 0.0f, 1.0f);
		float heightCoefficient = glm::sin(jumpCoefficient * glm::pi<float>());


		m_mesh->setHeightOffset(heightCoefficient * -jumpHeight);

		glm::vec3 fwd = m_targetRotation * e2::worldForwardf();

		m_movement->move(m_collision->radius(), glm::normalize(glm::vec2{ fwd.x, fwd.z }), float(seconds) * 1.0f, e2::CollisionType::All, m_collisionCache);

		if (m_jumpFactor >= jumpTime)
		{
			m_jumping = false;
			m_timeSinceJump = 0.0f;

			m_mesh->setPose("idle");
			m_mesh->playAction("land");
			playSound("S_Teardrop_Land.e2a", 0.5f, 1.0f);

		}
	}
	else
	{
		m_movement->resolve(m_collision->radius(), e2::CollisionType::All, m_collisionCache);

		m_mesh->setHeightOffset(0.0f);
		if (!m_mesh->isActionPlaying("jump"))
		{
			m_mesh->setPose("idle");
			float oldTimeSinceJump = m_timeSinceJump;
			m_timeSinceJump += float(seconds);

			if (oldTimeSinceJump < rotateTime && m_timeSinceJump >= rotateTime)
			{
				m_targetRotation = glm::angleAxis(glm::radians(e2::randomFloat(0.0f, 360.0f)), e2::worldUpf());
			}

			if (m_timeSinceJump > jumpInterval)
			{
				m_mesh->playAction("jump");
			}
		}
	}


	m_collision->invalidate();
	m_mesh->applyTransform();



}

void e2::AmberlingEntity::onTrigger(e2::Name action, e2::Name trigger)
{
	static const e2::Name triggerJump{ "jump" };
	if (trigger == triggerJump)
	{
		m_jumping = true;
		m_jumpFactor = 0.0f;
	}

	static const e2::Name triggerJumpSound{ "jump_sound" };
	if (trigger == triggerJumpSound)
	{
		playSound("S_Teardrop_Jump.e2a", 0.7f, 1.0f);
	}
}

void e2::AmberlingEntity::updateVisibility()
{
	m_mesh->updateVisibility();
}

void e2::AmberlingEntity::onMeleeDamage(e2::Entity* instigator, float dmg)
{
	if (m_health <= 0.0f)
		return;

	m_interacting = false;

	glm::vec2 pos = planarCoords();

	if (m_jumping)
	{
		game()->spawnHitLabel({ pos.x, -0.25f, pos.y }, "Miss!");
		return;
	}


	glm::vec2 instPos = instigator->planarCoords();
	m_hitDirection = glm::normalize(pos - instPos);

	game()->spawnHitLabel({ pos.x, -0.25f, pos.y }, std::format("{}", (uint32_t)dmg));
	m_health -= dmg;
	if (m_health <= 0.0f)
	{
		playSound("S_Teardrop_Die.e2a", 1.0, 0.5);
		m_mesh->playAction("die");
		game()->queueDestroyEntity(this);

		int64_t dropRate = e2::randomInt(0, 2);
		for (int64_t i = 0; i < dropRate; i++)
		{
			e2::ItemEntity *cube = game()->spawnEntity("radion_cube", getTransform()->getTranslation(e2::TransformSpace::World))->cast<e2::ItemEntity>();
			cube->setCooldown(0.5f);
		}
		
	}
	else
	{
		playSound("S_Teardrop_Hit.e2a", 1.0, 1.0);
		m_mesh->playAction("hit");
	}
}

bool e2::AmberlingEntity::canBeDestroyed()
{
	return !m_mesh->isActionPlaying("die");
}

void e2::AmberlingEntity::onInteract(e2::Entity* interactor)
{
	if (m_jumping)
	{
		glm::vec2 pos = planarCoords();
		game()->spawnHitLabel({ pos.x, -0.25f, pos.y }, "Miss!");
		return;
	}
	m_interacting = true;
	game()->runScriptGraph("test");
}



