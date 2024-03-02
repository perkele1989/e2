
#include "e2/managers/gamemanager.hpp"
#include "e2/game/gamesession.hpp"

#include "e2/application.hpp"

e2::GameManager::GameManager(Engine* owner)
	: e2::Manager(owner)
{

}

e2::GameManager::~GameManager()
{

}

void e2::GameManager::initialize()
{
	// Only create a default game session if the application type is "Game"
	// This is due to Tool applications setting their own stuff up, and creating sessions where they need to (like PIE for example)
	if(application()->type() == e2::ApplicationType::Game)
		m_session = new e2::GameSession(this);

}

void e2::GameManager::shutdown()
{
	if(m_session)
		delete m_session;
}

void e2::GameManager::update(double deltaTime)
{
	if (m_session)
		m_session->tick(deltaTime);
}
