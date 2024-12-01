
#include "game/gamecontext.hpp"

#include "game/game.hpp"

e2::HexGrid* e2::GameContext::hexGrid()
{
	return game()->m_hexGrid;
}

e2::GameSession* e2::GameContext::gameSession()
{
	return game()->m_session;
}

e2::RadionManager* e2::GameContext::radionManager()
{
	return game()->radionManager();
}
