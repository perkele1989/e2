
#pragma once 

#include <e2/buildcfg.hpp>
#include <e2/physics/physicscontext.hpp>


namespace e2
{
	class E2_API IPhysicsContext_Bt : public e2::IPhysicsContext
	{
	public:
		IPhysicsContext_Bt(e2::Context* engineContext);
		virtual ~IPhysicsContext_Bt();
		

		virtual IPhysicsWorld* createWorld(PhysicsWorldCreateInfo const& createInfo) override;


	private:

	};

}