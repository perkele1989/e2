
#include "e2/game/worldcontext.hpp"
#include "e2/game/world.hpp"

e2::Engine* e2::WorldContext::engine()
{
	return world()->engine();
}

e2::Session* e2::WorldContext::worldSession()
{
	return world()->worldSession();
}

