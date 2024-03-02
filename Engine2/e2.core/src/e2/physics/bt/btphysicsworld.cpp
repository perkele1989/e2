
#include "e2/physics/bt/btphysicsworld.hpp"
#include "e2/physics/bt/btphysicscontext.hpp"
#include "e2/physics/bt/btrigidbody.hpp"

e2::IPhysicsWorld_Bt::IPhysicsWorld_Bt(IPhysicsContext* context, e2::PhysicsWorldCreateInfo const& createInfo)
	: e2::IPhysicsWorld(context, createInfo)
	, e2::ContextHolder_Bt(context)
	/*, m_collisionConfiguration()
	, m_dispatcher(&m_collisionConfiguration)
	, m_pairCache()
	, m_constraintSolver()
	, m_world(&m_dispatcher, &m_pairCache, &m_constraintSolver, &m_collisionConfiguration)
	, m_fixedTimeStep(createInfo.fixedTimeStep)*/
{

}

e2::IPhysicsWorld_Bt::~IPhysicsWorld_Bt()
{

}

e2::IRigidBody* e2::IPhysicsWorld_Bt::createRigidBody(e2::RigidBodyCreateInfo const& createInfo)
{
	return e2::create<e2::IRigidBody_Bt>(this, createInfo);
}

void e2::IPhysicsWorld_Bt::tick(double seconds)
{
	//m_world.stepSimulation(seconds, 1, m_fixedTimeStep);
}
