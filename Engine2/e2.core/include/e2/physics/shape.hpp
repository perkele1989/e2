
#pragma once 

#include <e2/buildcfg.hpp>
#include <e2/export.hpp>
#include <e2/physics/physicsresource.hpp>

namespace e2
{

	class E2_API IShape : public e2::PhysicsResource
	{
		ObjectDeclaration()
	public:
		IShape(IPhysicsContext* ctx);
		virtual ~IShape();

	};
}

#include "shape.generated.hpp"