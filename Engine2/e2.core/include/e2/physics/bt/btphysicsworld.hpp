
#pragma once 

#include <e2/buildcfg.hpp>

#include <e2/physics/bt/btresource.hpp>
#include <e2/physics/physicsworld.hpp>

//#include <bullet/btBulletCollisionCommon.h>
//#include <bullet/btBulletDynamicsCommon.h>

namespace e2
{


	/** @tags(arena, arenaSize=32) */
	class E2_API IPhysicsWorld_Bt : public e2::IPhysicsWorld, public e2::ContextHolder_Bt
	{
		ObjectDeclaration()
	public:
		IPhysicsWorld_Bt(IPhysicsContext* context, e2::PhysicsWorldCreateInfo const& createInfo);
		virtual ~IPhysicsWorld_Bt();

		virtual IRigidBody* createRigidBody(e2::RigidBodyCreateInfo const& createInfo) override;

		virtual void tick(double seconds) override;
		/*
		btDefaultCollisionConfiguration m_collisionConfiguration;

		btCollisionDispatcher m_dispatcher;
		btDbvtBroadphase m_pairCache;
		btSequentialImpulseConstraintSolver m_constraintSolver;

		btDiscreteDynamicsWorld m_world;*/

		double m_fixedTimeStep{ 1.0 / 60.0 };
	};
}

#include "btphysicsworld.generated.hpp"