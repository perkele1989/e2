
#include "e2/physics/bt/btresource.hpp"
#include "e2/physics/bt/btphysicscontext.hpp"
#include "e2/physics/bt/btphysicsworld.hpp"

e2::ContextHolder_Bt::ContextHolder_Bt(e2::IPhysicsContext* ctx)
{
	m_physicsContextBt = static_cast<e2::IPhysicsContext_Bt*>(ctx);
}

e2::ContextHolder_Bt::~ContextHolder_Bt()
{

}

e2::WorldHolder_Bt::WorldHolder_Bt(e2::IPhysicsWorld* wrld)
	: e2::ContextHolder_Bt(wrld->physicsContext())
{
	m_physicsWorldBt = static_cast<e2::IPhysicsWorld_Bt*>(wrld);
}

e2::WorldHolder_Bt::~WorldHolder_Bt()
{

}
