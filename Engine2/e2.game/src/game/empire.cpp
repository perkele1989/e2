
#include "game/empire.hpp"
#include "game/game.hpp"

e2::GameEmpire::GameEmpire(e2::GameContext* ctx, uint8_t _id)
	: m_game(ctx->game())
	, id(_id)
{

}

e2::GameEmpire::GameEmpire()
{

}

e2::GameEmpire::~GameEmpire()
{

}

e2::Game* e2::GameEmpire::game()
{
	return m_game;
}

e2::CommanderAI::CommanderAI()
{

}

e2::CommanderAI::~CommanderAI()
{

}

void e2::CommanderAI::grugBrainTick()
{
	game()->endTurn();
}

e2::Game* e2::CommanderAI::game()
{
	return m_game;
}

e2::CommanderAI::CommanderAI(e2::GameContext* ctx, EmpireId empireId)
	: m_game(ctx->game())
	, id(empireId)
{

}
