
#pragma once 

#include <e2/buildcfg.hpp>
#include <e2/export.hpp>
#include <e2/physics/physicsresource.hpp>
#include <e2/physics/rigidbody.hpp>

namespace e2
{
	class IRigidBody;

	struct E2_API PhysicsWorldCreateInfo
	{
		double fixedTimeStep{ 1.0 / 60.0 };
	};

	class E2_API IPhysicsWorld : public e2::PhysicsResource
	{
		ObjectDeclaration()
	public:
		IPhysicsWorld(IPhysicsContext* ctx, PhysicsWorldCreateInfo const& createInfo);
		virtual ~IPhysicsWorld();

		virtual IRigidBody* createRigidBody(e2::RigidBodyCreateInfo const& createInfo) = 0;

		virtual void tick(double seconds) = 0;

	};
}

#include "physicsworld.generated.hpp"