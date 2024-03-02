
#include "e2/game/world.hpp"
#include "e2/game/session.hpp"
#include "e2/game/entity.hpp"
#include "e2/game/component.hpp"

e2::World::World(e2::Session* session)
	: m_session(session)
{
}

e2::World::~World()
{
	for (e2::Entity* e : m_newEntities)
	{
		e2::destroy(e);
	}
	for (e2::Entity* e : m_entities)
	{
		e2::destroy(e);
	}
}

e2::Engine* e2::World::engine()
{
	return m_session->engine();
}

void e2::World::tick(double seconds)
{
	// Spawn the new entities
	// Make sure we make a tmp set here and swap, since we wanna be able to spawn from spawn()
	{
		std::unordered_set<e2::Entity*> tmpSet;
		m_newEntities.swap(tmpSet);
		for (e2::Entity* newEntity : tmpSet)
		{
			m_entities.insert(newEntity);
			newEntity->onSpawned();
		}
	}

	for (e2::Entity* e : m_entities)
	{
		e->tick(seconds);
	}

	// Destroy the destroyed entities
	// Make sure we make a tmp set here and swap, since we wanna be able to destroy from destroy()
	{
		std::unordered_set<e2::Entity*> tmpSet;
		m_destroyedEntities.swap(tmpSet);
		for (e2::Entity* destroyedEntity : tmpSet)
		{
			destroyedEntity->onDestroyed();
			m_entities.erase(destroyedEntity);
			e2::destroy(destroyedEntity);
		}
	}
}

e2::Entity* e2::World::spawnEntity(e2::Name name, e2::Name typeName)
{
	e2::Entity* newEntity = e2::createDynamic(typeName)->cast<e2::Entity>();
	newEntity->postConstruct(this, name);
	m_newEntities.insert(newEntity);
	return newEntity;
}

e2::Session* e2::World::worldSession()
{
	return m_session;
}
