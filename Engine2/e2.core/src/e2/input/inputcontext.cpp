
#include "e2/input/inputcontext.hpp"

e2::IInputContext::IInputContext(e2::Context* engineContext)
	: m_engine(engineContext->engine())
{

}

e2::IInputContext::~IInputContext()
{

}

e2::Engine* e2::IInputContext::engine()
{
	return m_engine;
}
