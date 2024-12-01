
#pragma once 

#include <e2/context.hpp>

namespace e2
{
	class HexGrid;
	class Game;
	class GameSession;
	class RadionManager;

	class GameContext 
	{
	public:

		virtual e2::Game* game() = 0;

		virtual e2::HexGrid* hexGrid();
		virtual e2::GameSession* gameSession();
		e2::RadionManager* radionManager();
	};

}