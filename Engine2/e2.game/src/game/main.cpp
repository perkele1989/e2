
#include <e2/e2.hpp>

#include "game/game.hpp"

int main(int argc, char** argv)
{
	//_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF |  _CRTDBG_CHECK_ALWAYS_DF);
	e2::Engine engine;
	e2::Game game(&engine);

	engine.run(&game);

	return 0;
}