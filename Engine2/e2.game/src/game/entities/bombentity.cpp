
#include "game/entities/bombentity.hpp"


#include "game/entities/player.hpp"

#include "game/game.hpp"

#include <e2/game/gamesession.hpp>
#include <e2/e2.hpp>

#include <e2/utils.hpp>

e2::BombSpecification::BombSpecification()
	: e2::EntitySpecification()
{
}

e2::BombSpecification::~BombSpecification()
{
}

void e2::BombSpecification::populate(e2::GameContext* ctx, nlohmann::json& obj)
{
	e2::EntitySpecification::populate(ctx, obj);


	assets.insert("S_Bomb_Explode.e2a");

	if (obj.contains("isRedBomb"))
		isRedBomb = obj.at("isRedBomb").template get<bool>();

	if (obj.contains("mesh"))
		mesh.populate(obj.at("mesh"), assets);

	if (obj.contains("collision"))
		collision.populate(obj.at("collision"), assets);

	if (obj.contains("movement"))
		movement.populate(obj.at("movement"), assets);

}

void e2::BombSpecification::finalize()
{
	mesh.finalize(game);
	collision.finalize(game);
}





e2::BombEntity::BombEntity()
	: e2::ThrowableEntity()
{

}

e2::BombEntity::~BombEntity()
{
	if (m_mesh)
		e2::destroy(m_mesh);

	if (m_collision)
		e2::destroy(m_collision);

	if (m_movement)
		e2::destroy(m_movement);
}

void e2::BombEntity::postConstruct(e2::GameContext* ctx, e2::EntitySpecification* spec, glm::vec3 const& worldPosition, glm::quat const& worldRotation)
{

	e2::Entity::postConstruct(ctx, spec, worldPosition, worldRotation);
	m_bombSpecification = m_specification->cast<e2::BombSpecification>();

	m_mesh = e2::create<e2::StaticMeshComponent>(&m_bombSpecification->mesh, this);


	m_collision = e2::create<e2::CollisionComponent>(&m_bombSpecification->collision, this);
	m_movement = e2::create<e2::MovementComponent>(&m_bombSpecification->movement, this);

}

void e2::BombEntity::writeForSave(e2::IStream& toBuffer)
{
}

void e2::BombEntity::readForSave(e2::IStream& fromBuffer)
{
}

void e2::BombEntity::updateAnimation(double seconds)
{

}


void applyGravity(float& y, float& velocity, float deltaTime)
{
	constexpr float gravity = 9.8f;
	constexpr float restitution = 0.5f;

	y += (velocity * deltaTime) + (0.5f * gravity * deltaTime * deltaTime);
	velocity += gravity * deltaTime;

	if (y >= 0.0f)
	{
		y = 0.0f;
		velocity = -velocity * restitution;
	}
}

void e2::BombEntity::update(double seconds)
{
	m_collisionCache.clear();
	game()->populateCollisions(hex().offsetCoords(), e2::CollisionType::All, m_collisionCache);

	e2::PlayerEntity* player = game()->playerEntity();
	glm::vec2 playerCoords = player->planarCoords();
	glm::vec2 thisCoords = planarCoords();


	glm::vec2 xzVelocity{ m_velocity.x, m_velocity.z };
	float xzDistanceLeft = glm::length(xzVelocity);
	if (xzDistanceLeft > 0.01f)
	{
		glm::vec2 xzDir = glm::normalize(xzVelocity);


		constexpr float throwSpeed = 3.0;
		float throwDistance = throwSpeed * seconds;
		m_movement->move(m_collision->radius(), xzDir, throwDistance, e2::CollisionType::All, m_collisionCache);

		xzDistanceLeft -= throwDistance;
		xzVelocity = xzDir * xzDistanceLeft;

		m_velocity.x = xzVelocity.x;
		m_velocity.z = xzVelocity.y;
	}
	else
	{
		m_movement->resolve(m_collision->radius(), e2::CollisionType::All, m_collisionCache);
	}

	glm::vec3 worldPos = getTransform()->getTranslation(e2::TransformSpace::World);
	applyGravity(worldPos.y, m_velocity.y, seconds);
	getTransform()->setTranslation({ worldPos.x, worldPos.y, worldPos.z }, e2::TransformSpace::World);


	m_collision->invalidate();
	m_mesh->applyTransform();

	if (!m_bombSpecification->isRedBomb)
		m_life -= seconds;

	if (m_life <= 0.0f && !m_dead)
	{
		m_killTimer -= seconds;
		if (m_killTimer <= 0.0f)
		{
			game()->hexGrid()->grassCutMask().cut({ planarCoords(), 1.0f });
			static std::vector<e2::Collision> _collisions;
			_collisions.clear();
			game()->populateCollisions(hex().offsetCoords(), e2::CollisionType::Tree | e2::CollisionType::Component, _collisions, true);
			for (e2::Collision& col : _collisions)
			{
				if (col.type == e2::CollisionType::Tree)
				{
					float treeDistance = glm::distance(col.position, planarCoords());
					if (treeDistance < 0.8f)
					{
						game()->hexGrid()->damageTree(col.hex, col.treeIndex, 1.75f * (1.0 - glm::smoothstep(0.0f, 0.8f, treeDistance)));
					}
				}

				if (col.type == e2::CollisionType::Component)
				{
					if (col.component->entity() == this)
					{
						continue;
					}

					float cmpDistance = glm::distance(col.component->entity()->planarCoords(), planarCoords());
					if (cmpDistance < 0.8f)
					{
						col.component->entity()->onMeleeDamage(this, 40.0f * (1.0 - glm::smoothstep(0.0f, 0.8f, cmpDistance)));
					}
				}
			}

			game()->addScreenShake(0.3f);

			game()->queueDestroyEntity(this);

			playSound("S_Bomb_Explode.e2a", 1.0f, 0.9f);

			m_dead = true;
		}
	}



}


void e2::BombEntity::updateVisibility()
{
	m_mesh->updateVisibility();
}

void e2::BombEntity::onInteract(e2::Entity* instigator)
{
	if (!m_bombSpecification->isRedBomb)
		return;

	if (game()->playerState().give("redbomb"))
	{
		game()->queueDestroyEntity(this);
	}
	else
	{
		game()->pushMessage("Inventory full.");
	}
}

std::string e2::BombEntity::interactText()
{
	return "Pick up";
}

bool e2::BombEntity::interactable()
{
	return m_bombSpecification->isRedBomb;
}

void e2::BombEntity::onMeleeDamage(e2::Entity* instigator, float dmg)
{

	if (m_bombSpecification->isRedBomb)
	{
		m_life = 0.0f;
		return;
	}

	m_life -= (dmg / 37.5f) * 3.0f;
}

void e2::BombEntity::setVelocity(glm::vec3 newVelocity)
{
	m_velocity = newVelocity;
}
