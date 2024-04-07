
#include "e2/managers/asyncmanager.hpp"
#include "e2/timer.hpp"
#include <glm/glm.hpp>


namespace
{

}

e2::AsyncManager::AsyncManager(Engine* owner)
	: e2::Manager(owner)
{
	m_mainId = std::this_thread::get_id();
	
	// optimize for 4-16 cores 
	// 4 threads: worker gets 2
	// 8 threads: worker gets 3
	// 12 threads: worker gets 6 
	// 16 threads: worker gets 8 
	// if you change htis, make sure the above array of names have enough names for the threads
	uint32_t hwThreads = std::thread::hardware_concurrency();
	uint32_t numThreads = 1;

	if (hwThreads >= 6)
	{
		numThreads = hwThreads - 4;
	}
	else if (hwThreads >= 4)
	{
		numThreads = 2;
	}
	else
	{
		numThreads = 1;
	}

	m_threads.resize(numThreads, nullptr);
	for (uint32_t i = 0; i < numThreads; i++)
	{
		m_threads[i] = e2::create<e2::AsyncThread>(this, std::format("#{}", i));
		m_threads[i]->run();
	}

	m_queue.reserve(numThreads * 16);
	m_queueSwap.reserve(numThreads * 16);
}

e2::AsyncManager::~AsyncManager()
{
	for (e2::AsyncThread* thread : m_threads)
	{
		e2::destroy(thread);
	}
}

void e2::AsyncManager::initialize()
{
	m_partQueue.reserve(256);
	m_queue.reserve(256);
	m_queueSwap.reserve(256);
}

void e2::AsyncManager::shutdown()
{
	for (e2::AsyncThread* thread : m_threads)
	{
		thread->kill();
	}

	m_queueSwap.clear();
	m_queue.clear();
	m_partQueue.clear();
}

void e2::AsyncManager::preUpdate(double deltaTime)
{

}

void e2::AsyncManager::update(double deltaTime)
{
	m_queueSwap.clear();
	{
		std::scoped_lock lock(m_queueMutex);
		m_queue.swap(m_queueSwap);
	}

	for (e2::AsyncTaskPtr task : m_queueSwap)
	{
		task->prepare();
	}

	// @todo refactor so as to not use for loop. We need to iterate the threads with indices, and offset based
	// on last used thread, to make sure we have good CPU saturation with even thread distribution even for
	// cases where we most often have less jobs than threads (which is deemed to usually be the case)
	uint64_t tasksPerThread = uint64_t(glm::ceil(double(m_queueSwap.size()) / double(m_threads.size())));
	uint64_t numChunks = uint64_t(glm::ceil(double(m_queueSwap.size()) / double(tasksPerThread)));
	uint64_t offset = 0;
	uint64_t currChunk = 0;
	uint32_t lastIndex = m_lastThreadIndex;
	for(uint32_t i = 0; i < m_threads.size(); i++)
	//for (e2::AsyncThread* thread : m_threads)
	{
		if (currChunk >= numChunks)
			break;

		uint32_t correctedIndex = m_lastThreadIndex + 1 + i;
		while (correctedIndex >= m_threads.size())
		{
			correctedIndex -= (uint32_t)m_threads.size();
		}
		e2::AsyncThread* thread = m_threads[correctedIndex];

		if (offset + tasksPerThread >= m_queueSwap.size())
		{
			m_partQueue.insert(m_partQueue.end(), m_queueSwap.begin() + offset, m_queueSwap.end());
		}
		else
		{
			m_partQueue.insert(m_partQueue.end(), m_queueSwap.begin() + offset, m_queueSwap.begin() + offset + tasksPerThread);
		}

		thread->enqueue(m_partQueue);
		offset += tasksPerThread;
		currChunk++;
		m_lastThreadIndex = correctedIndex;

		m_partQueue.clear();
	}

	for(e2::AsyncThread *thread : m_threads)
	{
		for (e2::AsyncTaskPtr task : thread->fetch())
		{
			e2::AsyncTaskStatus currentStatus = task->status();
			if (currentStatus == AsyncTaskStatus::Processing)
			{
				if (task->finalize())
					task->status(e2::AsyncTaskStatus::Completed);
			}
		}
	}
}

void e2::AsyncManager::enqueue(std::vector<e2::AsyncTaskPtr> newTasks)
{
	std::scoped_lock lock(m_queueMutex);
	for (e2::AsyncTaskPtr &task : newTasks)
	{
		task->status(AsyncTaskStatus::Processing);
	}

	m_queue.insert(m_queue.end(), newTasks.begin(), newTasks.end());
}

bool e2::AsyncManager::isMainthread()
{
	return m_mainId == std::this_thread::get_id();
}

e2::AsyncThread::AsyncThread(e2::Context* ctx, e2::Name n)
	: m_engine(ctx->engine())
{
	m_name = n;
	m_fetchSwap.reserve(256);
	m_incoming.reserve(256);
	m_outgoing.reserve(256);
}

e2::AsyncThread::~AsyncThread()
{
	if (m_running)
	{
		kill();
	}
}

void e2::AsyncThread::run()
{
	m_thread = std::thread([this]() {
		m_running = true;
		uint64_t framesSinceActivity{};

		std::vector<e2::AsyncTaskPtr> incomingSwap;

		e2::Timer asyncTimer;

		while (m_running)
		{
			incomingSwap.clear();
			{
				std::scoped_lock lock(m_incomingMutex);
				m_incoming.swap(incomingSwap);
			}

			if (incomingSwap.empty())
				framesSinceActivity++;
			else
				framesSinceActivity = 0;

			for (e2::AsyncTaskPtr task : incomingSwap)
			{
				task->setThreadName(m_name);
				asyncTimer.reset();
				task->execute();
				task->setAsyncTime(asyncTimer.seconds() * 1000.0);

				{
					std::scoped_lock lock(m_outgoingMutex);
					m_outgoing.push_back(task);
				}
			}

			// Progressively sleep more when more idle
			if (framesSinceActivity >= 128)
				std::this_thread::sleep_for(std::chrono::milliseconds(256));
			else if(framesSinceActivity >= 16)
				std::this_thread::sleep_for(std::chrono::milliseconds(32));
		}
	});
}

void e2::AsyncThread::kill()
{
	m_running = false;
	m_thread.join();

	m_fetchSwap.clear();
}

void e2::AsyncThread::enqueue(std::vector<e2::AsyncTaskPtr> &newTasks)
{
	std::scoped_lock lock(m_incomingMutex);
	m_incoming.insert(m_incoming.end(), newTasks.begin(), newTasks.end());
}

std::vector<e2::AsyncTaskPtr> &e2::AsyncThread::fetch()
{
	m_fetchSwap.clear();
	{
		std::scoped_lock lock(m_outgoingMutex);
		m_outgoing.swap(m_fetchSwap);
	}

	return m_fetchSwap;
}

