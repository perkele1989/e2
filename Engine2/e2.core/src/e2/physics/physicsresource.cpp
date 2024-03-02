
#include "e2/physics/physicsresource.hpp"
#include "e2/physics/physicscontext.hpp"
#include "e2/physics/physicsworld.hpp"

e2::PhysicsResource::PhysicsResource(IPhysicsContext* context)
	: m_physicsContext(context)
{

}

e2::PhysicsResource::~PhysicsResource()
{

}


e2::IPhysicsContext* e2::PhysicsResource::physicsContext() const
{
	return m_physicsContext;
}

e2::PhysicsWorldResource::PhysicsWorldResource(IPhysicsWorld* world)
	: e2::PhysicsResource(world->physicsContext())
	, m_physicsWorld(world)
{

}

e2::PhysicsWorldResource::~PhysicsWorldResource()
{

}

e2::IPhysicsWorld* e2::PhysicsWorldResource::physicsWorld() const
{
	return m_physicsWorld;
}