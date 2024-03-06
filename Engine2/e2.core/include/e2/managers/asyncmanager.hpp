
#pragma once 

#include <e2/export.hpp>
#include <e2/manager.hpp>

#include <e2/async.hpp>

#include <thread>
#include <memory>
#include <mutex>
#include <cstdint>
#include <queue>

namespace e2
{
	class Engine;


	/**  */
	class E2_API AsyncThread : public e2::Context
	{
	public:
		AsyncThread(e2::Context* ctx, e2::Name n);
		virtual ~AsyncThread();

		void run();
		void kill();


		void enqueue(std::vector<e2::AsyncTaskPtr> &newTasks);

		std::vector<e2::AsyncTaskPtr> &fetch();

		virtual Engine* engine() override
		{
			return m_engine;
		}

		inline e2::Name name() const
		{
			return m_name;
		}

	protected:
		e2::Engine* m_engine{};
		std::thread m_thread;
		e2::Name m_name;
		bool m_running{};

		std::mutex m_incomingMutex;
		std::vector<e2::AsyncTaskPtr> m_incoming;

		std::mutex m_outgoingMutex;
		std::vector<e2::AsyncTaskPtr> m_outgoing;

		std::vector<e2::AsyncTaskPtr> m_fetchSwap;
	};

	class E2_API AsyncManager : public Manager
	{
	public:
		AsyncManager(Engine* owner);
		virtual ~AsyncManager();

		virtual void initialize() override;
		virtual void shutdown() override;

		virtual void preUpdate(double deltaTime) override;
		virtual void update(double deltaTime) override;


		void enqueue(std::vector<e2::AsyncTaskPtr> newTasks);


		bool isMainthread();
	protected:

		std::thread::id m_mainId;

		uint32_t m_lastThreadIndex{};

		std::vector<e2::AsyncThread*> m_threads;
		std::mutex m_queueMutex;
		std::vector<e2::AsyncTaskPtr> m_queue;
		std::vector<e2::AsyncTaskPtr> m_queueSwap;

		std::vector<e2::AsyncTaskPtr> m_partQueue;
	};

}