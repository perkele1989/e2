
#pragma once 

#include <e2/export.hpp>

namespace e2
{
	class IPhysicsContext;
	class IPhysicsContext_Bt;
	class IPhysicsWorld;
	class IPhysicsWorld_Bt;


	class E2_API ContextHolder_Bt
	{
	public:
		ContextHolder_Bt(e2::IPhysicsContext* ctx);
		virtual ~ContextHolder_Bt();

		IPhysicsContext_Bt* m_physicsContextBt{};
	};

	class E2_API WorldHolder_Bt : public e2::ContextHolder_Bt
	{
	public:
		WorldHolder_Bt(e2::IPhysicsWorld* wrld);
		virtual ~WorldHolder_Bt();

		IPhysicsWorld_Bt* m_physicsWorldBt{};
	};
}