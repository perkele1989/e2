
#include "e2/physics/bt/btshape.hpp"

e2::IShape_Bt::IShape_Bt(IPhysicsContext* ctx)
	: e2::IShape(ctx)
	, e2::ContextHolder_Bt(ctx)
{

}

e2::IShape_Bt::~IShape_Bt()
{

}

