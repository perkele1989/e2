
#pragma once 

#include <e2/buildcfg.hpp>

#include <e2/physics/bt/btresource.hpp>
#include <e2/physics/shape.hpp>

//#include <bullet/btBulletCollisionCommon.h>

namespace e2
{


	class E2_API IShape_Bt : public e2::IShape, public e2::ContextHolder_Bt
	{
		ObjectDeclaration()
	public:
		IShape_Bt(IPhysicsContext* ctx);
		virtual ~IShape_Bt();

		//btCollisionShape* m_shape{};
	};
}

#include "btshape.generated.hpp"