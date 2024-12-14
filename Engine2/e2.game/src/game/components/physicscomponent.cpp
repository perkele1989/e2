

#include "game/components/physicscomponent.hpp"
#include "game/entity.hpp"
#include "game/game.hpp"

#include <e2/game/gamesession.hpp>

e2::CollisionComponent::CollisionComponent(e2::CollisionSpecification* specification, e2::Entity* entity)
	: m_specification(specification)
	, m_entity(entity)
{
	e2::Session* session = m_entity->gameSession();
	m_currentIndex = entity->hex().offsetCoords();
	m_radius = m_specification->radius;
	m_entity->game()->registerCollisionComponent(this, m_currentIndex);
	block = specification->block;
}

e2::CollisionComponent::~CollisionComponent()
{
	m_entity->game()->unregisterCollisionComponent(this, m_currentIndex);
}


void e2::CollisionComponent::invalidate()
{
	glm::ivec2 newIndex = m_entity->hex().offsetCoords();
	if (newIndex != m_currentIndex)
	{
		m_entity->game()->unregisterCollisionComponent(this, m_currentIndex);
		m_currentIndex = newIndex;
		m_entity->game()->registerCollisionComponent(this, m_currentIndex);
	}

	if(block)
		m_entity->game()->hexGrid()->grassCutMask().push({ m_entity->planarCoords(), m_radius*4.0f });
}

void e2::CollisionComponent::setRadius(float newRadius)
{
	m_radius = newRadius;
	invalidate();
}


void e2::CollisionSpecification::populate(nlohmann::json& obj, std::unordered_set<e2::Name>& deps)
{
	if (obj.contains("radius"))
	{
		radius = obj.at("radius").template get<float>();
	}

	if (obj.contains("block"))
	{
		block = obj.at("block").template get<bool>();
	}

}

void e2::CollisionSpecification::finalize(e2::GameContext* ctx)
{

}

//
//
//void e2::MovementSpecification::populate(nlohmann::json& obj, std::unordered_set<e2::Name>& deps)
//{
//}
//
//void e2::MovementSpecification::finalize(e2::GameContext* ctx)
//{
//}

e2::MovementComponent::MovementComponent(e2::MovementSpecification* specification, e2::Entity* entity)
	: m_entity(entity)
	, m_specification(specification)
{
}

e2::MovementComponent::~MovementComponent()
{
}

float e2::MovementComponent::move(float radius, glm::vec2 const& dir, float energy, e2::CollisionType mask, std::vector<e2::Collision> const& collisions)
{
	glm::vec3 worldPosition = m_entity->getTransform()->getTranslation(e2::TransformSpace::World);
	glm::vec2 planarPosition = glm::vec2(worldPosition.x, worldPosition.z);
	//e2::Hex c(planarPosition);

	// actually start sweepin


	//gameSession()->renderer()->debugCircle(glm::vec3{ 0.0f, 0.0f, 1.0f }, m_position, 0.05f);

	constexpr float epsilon = 0.001f;

	float energyLeft = energy; // movespeed * seconds
	glm::vec2 moveDirection = dir;
	glm::vec2 startPosition = planarPosition;
	glm::vec2 targetPosition = planarPosition + (moveDirection * energyLeft);

	constexpr uint32_t maxSolverSteps = 2;
	for (uint32_t i = 0; i < maxSolverSteps; i++)
	{

		e2::SweepResult closestResult;
		closestResult.moveDistance = std::numeric_limits<float>::max();
		for (e2::Collision const& c : collisions)
		{
			if ((c.type & mask) != c.type && c.type != e2::CollisionType::Component)
				continue;

			if (c.type == e2::CollisionType::Component && c.component && ( !c.component->block ||  shouldIgnore(c.component->entity()) ))
				continue;

			e2::SweepResult result = e2::circleSweepTest(startPosition, targetPosition, radius, c.position, c.radius);
			if (result.hit)
			{
				if (result.moveDistance < closestResult.moveDistance && (c.type & mask) == c.type)
				{
					closestResult = result;
				}

				if (c.type == e2::CollisionType::Component)
				{
					m_entity->onHitEntity(c.component->entity());
				}
			}
		}

		float distanceToMove = energyLeft;

		if (closestResult.hit)
			distanceToMove = closestResult.moveDistance -epsilon;

		// move this step
		planarPosition += moveDirection * distanceToMove;
		energyLeft -= distanceToMove;
		if (energyLeft <= 0.0f)
			break;

		if (closestResult.hit)
		{
			glm::vec2 obstacleToStop = glm::normalize(closestResult.stopLocation - closestResult.obstacleLocation);
			glm::vec2 otsPerpendicular = glm::vec2(obstacleToStop.y, -obstacleToStop.x);
			otsPerpendicular *= glm::sign(glm::dot(otsPerpendicular, closestResult.stopLocation - startPosition));

			moveDirection = otsPerpendicular;
			energyLeft *= glm::max(0.f, glm::dot(otsPerpendicular, dir));
			if (energyLeft <= 0.0f)
			{
				break;
			}
		}
		startPosition = planarPosition;
		targetPosition = startPosition + moveDirection * energyLeft;
	}

	for (e2::Collision const& c : collisions)
	{
		if ((c.type & mask) != c.type)
			continue;

		if (c.type == e2::CollisionType::Component && c.component && (!c.component->block || shouldIgnore(c.component->entity())))
			continue;

		glm::vec2 cToThis = planarPosition - c.position;
		float distance = glm::length(cToThis);
		cToThis = glm::normalize(cToThis);
		bool intersect = distance < c.radius + radius;
		if (intersect)
		{
			planarPosition = c.position + cToThis * (c.radius + radius + 0.001f);
		}
	}

	m_entity->getTransform()->setTranslation(glm::vec3(planarPosition.x, worldPosition.y, planarPosition.y), e2::TransformSpace::World);

	if (energyLeft < 0.0f)
		energyLeft = 0.0f;

	return energyLeft;
}

void e2::MovementComponent::resolve(float radius, e2::CollisionType mask, std::vector<e2::Collision> const& collisions)
{
	glm::vec3 worldPosition = m_entity->getTransform()->getTranslation(e2::TransformSpace::World);
	glm::vec2 planarPosition = glm::vec2(worldPosition.x, worldPosition.z);

	for (e2::Collision const& c : collisions)
	{
		if ((c.type & mask) != c.type)
			continue;

		if (c.type == e2::CollisionType::Component && c.component && (!c.component->block || shouldIgnore(c.component->entity())))
			continue;

		if (planarPosition == c.position)
		{
			planarPosition.y += 0.001f;
		}

		glm::vec2 cToThis = planarPosition - c.position;
		float distance = glm::length(cToThis);
		cToThis = glm::normalize(cToThis);
		bool intersect = distance < c.radius + radius;
		if (intersect)
		{
			planarPosition = c.position + cToThis * (c.radius + radius + 0.001f);
		}
	}

	m_entity->getTransform()->setTranslation(glm::vec3(planarPosition.x, worldPosition.y, planarPosition.y), e2::TransformSpace::World);

}

bool e2::MovementComponent::shouldIgnore(e2::Entity* entity)
{
	return entity == m_entity || entity->pendingDestroy || m_specification->ignoredTypes.contains(entity->type());
}

void e2::MovementSpecification::populate(nlohmann::json& obj, std::unordered_set<e2::Name>& deps)
{
	if (obj.contains("ignoreTypes"))
	{
		for (nlohmann::json& ignore : obj.at("ignoreTypes"))
		{
			e2::Type const* t = e2::Type::fromName(ignore.template get<std::string>());
			if (t)
				ignoredTypes.insert(t);
		}
	}
}

void e2::MovementSpecification::finalize(e2::GameContext* ctx)
{
}
