
#include "e2/physics/bt/btphysicscontext.hpp"
#include "e2/physics/bt/btphysicsworld.hpp"

e2::IPhysicsContext_Bt::IPhysicsContext_Bt(e2::Context* engineContext)
	: e2::IPhysicsContext(engineContext)
{

}

e2::IPhysicsContext_Bt::~IPhysicsContext_Bt()
{

}

e2::IPhysicsWorld* e2::IPhysicsContext_Bt::createWorld(PhysicsWorldCreateInfo const& createInfo)
{
	return e2::create<e2::IPhysicsWorld_Bt>(this, createInfo);
}
