
#include "e2/physics/rigidbody.hpp"

e2::IRigidBody::IRigidBody(IPhysicsWorld* wrld, RigidBodyCreateInfo const& createInfo)
	: e2::PhysicsWorldResource(wrld)
{

}

e2::IRigidBody::~IRigidBody()
{

}
