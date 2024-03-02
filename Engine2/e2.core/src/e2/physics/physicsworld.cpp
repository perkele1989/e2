
#include "e2/physics/physicsworld.hpp"

e2::IPhysicsWorld::IPhysicsWorld(IPhysicsContext* ctx, PhysicsWorldCreateInfo const& createInfo)
	: e2::PhysicsResource(ctx)
{

}

e2::IPhysicsWorld::~IPhysicsWorld()
{

}