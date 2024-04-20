
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

void e2::GameEmpire::write(Buffer& destination) const
{
	destination << resources;
}

bool e2::GameEmpire::read(Buffer& source)
{
	source >> resources;

	return true;
}

e2::EmpireAI::EmpireAI()
{

}

e2::EmpireAI::~EmpireAI()
{

}

void e2::EmpireAI::grugBrainWakeUp()
{

}

void e2::EmpireAI::grugBrainTick(double seconds)
{
	game()->endTurn();
}

void e2::EmpireAI::grugBrainGoSleep()
{

}

e2::Game* e2::EmpireAI::game()
{
	return m_game;
}

e2::EmpireAI::EmpireAI(e2::GameContext* ctx, EmpireId empireId)
	: m_game(ctx->game())
	, id(empireId)
{

}

e2::NomadAI::NomadAI()
	: e2::EmpireAI()
{

}

e2::NomadAI::~NomadAI()
{

}

void e2::NomadAI::grugBrainWakeUp()
{

}

void e2::NomadAI::grugBrainTick(double seconds)
{

}

void e2::NomadAI::grugBrainGoSleep()
{

}

e2::NomadAI::NomadAI(e2::GameContext* ctx, EmpireId empireId)
	: e2::EmpireAI(ctx, empireId)
{

}

e2::CommanderAI::CommanderAI()
	: e2::EmpireAI()
{

}

e2::CommanderAI::~CommanderAI()
{

}

void e2::CommanderAI::grugBrainTick(double seconds)
{

}

e2::CommanderAI::CommanderAI(e2::GameContext* ctx, EmpireId empireId)
	: e2::EmpireAI(ctx, empireId)
{

}
