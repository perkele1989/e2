
#include "e2/game/entity.hpp"
#include "e2/game/world.hpp"
#include "e2/game/component.hpp"

e2::Entity::Entity(e2::World* world, e2::Name name)
	: m_world(world)
	, m_name(name)
{
	m_transform = e2::create<e2::Transform>();
}

e2::Entity::Entity()
{
	m_transform = e2::create<e2::Transform>();
}

e2::Entity::~Entity()
{
	for (e2::Component* c : m_newComponents)
	{
		e2::destroy(c);
	}

	for (e2::Component* c : m_components)
	{
		e2::destroy(c);
	}

	e2::destroy(m_transform);

}

void e2::Entity::postConstruct(e2::World* world, e2::Name name)
{
	m_world = world;
	m_name = name;

}

void e2::Entity::onSpawned()
{

}

void e2::Entity::onDestroyed()
{

}

e2::World* e2::Entity::world()
{
	return m_world;
}

void e2::Entity::tick(double seconds)
{
	// Spawn the new components
	// Make sure we make a tmp set here and swap, since we wanna be able to spawn from spawn()
	{
		std::unordered_set<e2::Component*> tmpSet;
		m_newComponents.swap(tmpSet);
		for (e2::Component* newComponent : tmpSet)
		{
			m_components.insert(newComponent);
			newComponent->onSpawned();
		}
	}

	for (e2::Component* c : m_components)
	{
		c->tick(seconds);
	}

	// Destroy the destroyed components
	// Make sure we make a tmp set here and swap, since we wanna be able to destroy from destroy()
	{
		std::unordered_set<e2::Component*> tmpSet;
		m_destroyedComponents.swap(tmpSet);
		for (e2::Component *destroyedComponent : tmpSet)
		{
			destroyedComponent->onDestroyed();
			m_components.erase(destroyedComponent);
			e2::destroy(destroyedComponent);
		}
	}
}

e2::Component* e2::Entity::spawnComponent(e2::Name name, e2::Name typeName)
{
	e2::Component* newComponent = e2::createDynamic(typeName)->cast<e2::Component>();
	newComponent->postConstruct(this, name);
	m_newComponents.insert(newComponent);
	return newComponent;
}
