
#include "e2/application.hpp"
#include "e2/e2.hpp"

#include "e2/game/meshcomponent.hpp"

e2::Application::Application(e2::Context* ctx)
	: m_engine(ctx->engine())
{

}

e2::Application::~Application()
{

}


e2::Engine* e2::Application::engine()
{
	return m_engine;
}
