
#include "game/empire.hpp"
#include "game/game.hpp"

e2::GameEmpire::GameEmpire(e2::GameContext* ctx, EmpireId _id)
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
	for (e2::GameEntity* ent : empire->entities)
	{
		if (ent->grugRelevant())
		{
			turnEntities.insert(ent);
		}
	}

	cycleNextEntity();
}

void e2::EmpireAI::grugBrainTick(double seconds)
{
	if (!currentEntity)
	{
		game()->endTurn();
		return;
	}

	if (!game()->entityRelevantForPlay(currentEntity) || !currentEntity->grugTick(seconds))
	{
		cycleNextEntity();
	}
}

void e2::EmpireAI::grugBrainGoSleep()
{

}

void e2::EmpireAI::cycleNextEntity()
{
	do
	{
		if (currentEntity)
			turnEntities.erase(currentEntity);

		currentEntity = nullptr;

		if (turnEntities.size() > 0)
			currentEntity = *turnEntities.begin();
	} while (currentEntity && !game()->entityRelevantForPlay(currentEntity));

	if (currentEntity)
		game()->selectEntity(currentEntity);
	else
		game()->deselectEntity();
}

e2::Game* e2::EmpireAI::game()
{
	return m_game;
}

e2::EmpireAI::EmpireAI(e2::GameContext* ctx, EmpireId empireId)
	: m_game(ctx->game())
	, id(empireId)
{
	empire = m_game->empireById(id);
}

e2::NomadAI::NomadAI()
	: e2::EmpireAI()
{

}

e2::NomadAI::~NomadAI()
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

e2::CommanderAI::CommanderAI(e2::GameContext* ctx, EmpireId empireId)
	: e2::EmpireAI(ctx, empireId)
{

}
