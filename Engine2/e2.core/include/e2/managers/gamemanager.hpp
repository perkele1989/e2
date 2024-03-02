
#pragma once 

#include <e2/export.hpp>
#include <e2/manager.hpp>

namespace e2
{
	class Engine;
	class GameSession;

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

	protected:
		e2::GameSession* m_session{};
	};

}