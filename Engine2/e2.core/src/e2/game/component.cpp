
#include "e2/game/component.hpp"
#include "e2/game/entity.hpp"

e2::Component::Component(e2::Entity* entity, e2::Name name)
	: m_entity(entity)
	, m_name(name)
{

}

e2::Component::~Component()
{

}

void e2::Component::postConstruct(e2::Entity* entity, e2::Name name)
{
	m_entity = entity;
	m_name = name;
}

void e2::Component::onSpawned()
{

}

void e2::Component::tick(double seconds)
{

}

void e2::Component::onDestroyed()
{

}

e2::World* e2::Component::world()
{
	return m_entity->world();
}
