
#pragma once 

#include <e2/export.hpp>
#include <e2/manager.hpp>

#include <unordered_set>

namespace e2
{
	class Engine;
	class GameSession;
	class Session;

	class E2_API GameManager : public Manager
	{
	public:
		GameManager(Engine* owner);
		virtual ~GameManager();

		virtual void initialize() override;
		virtual void shutdown() override;


		virtual void update(double deltaTime) override;

		inline e2::GameSession* session() const
		{
			return m_session;
		}

		void registerSession(e2::Session* session);
		void unregisterSession(e2::Session* session);

		std::unordered_set<e2::Session*> & allSessions();

	protected:
		e2::GameSession* m_session{};

		std::unordered_set<e2::Session*> m_sessions;
	};

}