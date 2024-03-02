
#include "e2/managers/physicsmanager.hpp"

#include "e2/physics/bt/btphysicscontext.hpp"

e2::PhysicsManager::PhysicsManager(Engine* owner)
	: e2::Manager(owner)
{
	m_physicsContext = e2::create<e2::IPhysicsContext_Bt>(this);
}

e2::PhysicsManager::~PhysicsManager()
{
	for (e2::IPhysicsWorld* wrld : m_worlds)
		e2::destroy(wrld);

	e2::destroy(m_physicsContext);
}

void e2::PhysicsManager::initialize()
{

}

void e2::PhysicsManager::shutdown()
{

}

void e2::PhysicsManager::update(double deltaTime)
{

}

e2::IPhysicsWorld* e2::PhysicsManager::createWorld(e2::PhysicsWorldCreateInfo const& createInfo)
{
	e2::IPhysicsWorld* newWorld = m_physicsContext->createWorld(createInfo);
	m_worlds.push(newWorld);
	return newWorld;
}

void e2::PhysicsManager::destroyWorld(e2::IPhysicsWorld* world)
{
	m_worlds.removeFirstByValueFast(world);
	e2::destroy(world);
}
