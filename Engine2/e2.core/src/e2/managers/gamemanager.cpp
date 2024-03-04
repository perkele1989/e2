
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
		m_session = e2::create<e2::GameSession>(this);

}

void e2::GameManager::shutdown()
{
	if(m_session)
		e2::destroy(m_session);
}

void e2::GameManager::update(double deltaTime)
{
	if (m_session)
		m_session->tick(deltaTime);
}

void e2::GameManager::registerSession(e2::Session* session)
{
	m_sessions.insert(session);
}

void e2::GameManager::unregisterSession(e2::Session* session)
{
	m_sessions.erase(session);
}

std::unordered_set<e2::Session*> & e2::GameManager::allSessions()
{
	return m_sessions;
}
