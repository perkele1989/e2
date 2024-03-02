
#pragma once

#include "e2/context.hpp"
#include "e2/utils.hpp"

#include <atomic>
#include <memory>

namespace e2
{

	enum class AsyncTaskStatus : uint8_t
	{
		New,
		Processing,
		Completed,
		Failed
	};

	class E2_API AsyncTask : public e2::Context, public e2::ManagedObject
	{
		ObjectDeclaration()
	public:
		AsyncTask(e2::Context* context);
		virtual ~AsyncTask();

		virtual bool prepare() { return true; }
		virtual bool execute() { return true; }
		virtual bool finalize() { return true; }

		e2::AsyncTaskStatus status();
		void status(e2::AsyncTaskStatus  newStatus);

		virtual Engine* engine() override;

		void setThreadName(e2::Name newName);
		void setAsyncTime(double newTime);

	protected:

		e2::Engine* m_engine{};

		// Name this thread has been executed on, only valid from just before execute has been invoked
		e2::Name m_threadName;  

		// Execution time in milliseconds, only valid after execute has been called
		double m_asyncTime{};

		std::atomic_uint8_t m_status{ uint8_t(e2::AsyncTaskStatus::New)};
	};
} 
 
#include "async.generated.hpp"