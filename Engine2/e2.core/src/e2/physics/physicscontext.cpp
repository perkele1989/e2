
#include "e2/physics/physicscontext.hpp"

e2::IPhysicsContext::IPhysicsContext(e2::Context* engineContext)
	: m_engine(engineContext->engine())
{

}

e2::IPhysicsContext::~IPhysicsContext()
{

}

e2::Engine* e2::IPhysicsContext::engine()
{
	return m_engine;
}
