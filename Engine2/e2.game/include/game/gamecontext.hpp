
#pragma once 

namespace e2
{
	class HexGrid;
	class Game;
	class GameSession;

	class GameContext
	{
	public:
		virtual e2::Game* game() = 0;

		virtual e2::HexGrid* hexGrid();
		virtual e2::GameSession* gameSession();
	};

}