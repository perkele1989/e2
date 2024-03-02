
#include "e2/physics/bt/btrigidbody.hpp"
#include "e2/physics/bt/btphysicsworld.hpp"
#include "e2/physics/bt/btshape.hpp"

namespace
{
	/*
	btTransform toBtTransform(glm::mat4 const& in)
	{
		btTransform out;
		// @tdoo
		return out;
	}

	btCollisionShape* toBtShape(e2::IShape* shape)
	{
		e2::IShape_Bt* btShape = static_cast<e2::IShape_Bt*>(shape);
		return btShape->m_shape;
	}

	btVector3 toBtInertia(e2::IShape* shape, double mass)
	{
		if (mass == 0.0)
			return btVector3(0.0, 0.0, 0.0);

		e2::IShape_Bt* btShape = static_cast<e2::IShape_Bt*>(shape);

		btVector3 returner;
		btShape->m_shape->calculateLocalInertia(mass, returner);
		return returner;
	}
	*/
}

e2::IRigidBody_Bt::IRigidBody_Bt(IPhysicsWorld* world, e2::RigidBodyCreateInfo const& createInfo)
	: IRigidBody (world, createInfo)
	, WorldHolder_Bt(world)
	//, m_motionState(::toBtTransform(createInfo.initialTransform), ::toBtTransform(createInfo.comOffset))
	//, m_body(createInfo.mass, &m_motionState, ::toBtShape(createInfo.shape), ::toBtInertia(createInfo.shape, createInfo.mass))
{
	//e2::IPhysicsWorld_Bt* btWorld = static_cast<e2::IPhysicsWorld_Bt*>(world);
	//btWorld->m_world.addRigidBody(&m_body, createInfo.group, createInfo.mask);
}

e2::IRigidBody_Bt::~IRigidBody_Bt()
{
	//m_physicsWorldBt->m_world.removeRigidBody(&m_body);
}
