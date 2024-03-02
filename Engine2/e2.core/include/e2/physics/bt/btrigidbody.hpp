
#pragma once 

#include <e2/buildcfg.hpp>

#include <e2/physics/bt/btresource.hpp>
#include <e2/physics/physicsworld.hpp>

//#include <bullet/btBulletCollisionCommon.h>
//#include <bullet/btBulletDynamicsCommon.h>

namespace e2
{


	/** @tags(arena, arenaSize=e2::maxNumRigidBodies) */
	class E2_API IRigidBody_Bt : public e2::IRigidBody, public e2::WorldHolder_Bt
	{
		ObjectDeclaration()
	public:
		IRigidBody_Bt(IPhysicsWorld* world, e2::RigidBodyCreateInfo const& createInfo);
		virtual ~IRigidBody_Bt();

		//btDefaultMotionState m_motionState;
		//btRigidBody m_body;
	};
}

#include "btrigidbody.generated.hpp"