
#pragma once 

#include <e2/buildcfg.hpp>
#include <e2/export.hpp>
#include <e2/context.hpp>
#include <e2/physics/physicsworld.hpp>

namespace e2
{

	class E2_API IPhysicsContext : public Context
	{
	public:
		IPhysicsContext(e2::Context* engineContext);
		virtual ~IPhysicsContext();


		virtual IPhysicsWorld* createWorld(PhysicsWorldCreateInfo const& createInfo) = 0;

		virtual Engine* engine() override;

	protected:
		e2::Engine* m_engine{};
	};
}