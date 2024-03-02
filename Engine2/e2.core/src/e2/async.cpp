
#include "e2/async.hpp"

e2::AsyncTask::AsyncTask(e2::Context* context)
	: m_engine(context->engine())
{


}

e2::AsyncTask::~AsyncTask()
{

}


e2::AsyncTaskStatus e2::AsyncTask::status()
{
	return (e2::AsyncTaskStatus)m_status.load();
}

e2::Engine* e2::AsyncTask::engine()
{
	return m_engine;
}

void e2::AsyncTask::setThreadName(e2::Name newName)
{
	m_threadName = newName;
}

void e2::AsyncTask::setAsyncTime(double newTime)
{
	m_asyncTime = newTime;
}

void e2::AsyncTask::status(e2::AsyncTaskStatus  newStatus)
{
	m_status.store(uint8_t(newStatus));

}