
#include "game/empire.hpp"

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

