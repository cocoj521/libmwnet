#include "MWThreadPool.h"
#include <mwnet_mt/base/ThreadPool.h>

using namespace mwnet_mt;
using namespace MWTHREADPOOL;

typedef std::shared_ptr<mwnet_mt::ThreadPool> ThreadPoolPtr;

namespace MWTHREADPOOL
{
ThreadPool::ThreadPool(const std::string& poolname)
{
	ThreadPoolPtr pPool(new mwnet_mt::ThreadPool(poolname));
	m_any = pPool;
}

ThreadPool::~ThreadPool()
{
	
}

void ThreadPool::SetMaxTaskQueueSize(int maxquesize)
{
	ThreadPoolPtr pPool(boost::any_cast<ThreadPoolPtr>(m_any));
	if (pPool)
	{
		pPool->setMaxQueueSize(maxquesize<0?0:maxquesize);
	}
}

void ThreadPool::SetThreadInitCallback(const THREAD_TASK& thrinitcb)
{
	ThreadPoolPtr pPool(boost::any_cast<ThreadPoolPtr>(m_any));
	if (pPool)
	{
		pPool->setThreadInitCallback(thrinitcb);
	}
}

void ThreadPool::StartThreadPool(int thrnum)
{
	ThreadPoolPtr pPool(boost::any_cast<ThreadPoolPtr>(m_any));
	if (pPool)
	{
		pPool->start(thrnum<0?0:thrnum);
	}
}

void ThreadPool::StopThreadPool()
{
	ThreadPoolPtr pPool(boost::any_cast<ThreadPoolPtr>(m_any));
	if (pPool)
	{
		pPool->stop();
	}	
}

const char* ThreadPool::TheadPoolName()
{
	ThreadPoolPtr pPool(boost::any_cast<ThreadPoolPtr>(m_any));
	if (pPool)
	{
		return pPool->name().c_str();
	}
	return "";
}

size_t ThreadPool::TaskQueueSize() const
{
	ThreadPoolPtr pPool(boost::any_cast<ThreadPoolPtr>(m_any));
	if (pPool)
	{
		return pPool->queueSize();
	}
	return 0;
}

void ThreadPool::RunTask(const THREAD_TASK& task)
{
	ThreadPoolPtr pPool(boost::any_cast<ThreadPoolPtr>(m_any));
	if (pPool)
	{
		pPool->run(task);
	}
}

bool ThreadPool::TryRunTask(const THREAD_TASK& task)
{
	ThreadPoolPtr pPool(boost::any_cast<ThreadPoolPtr>(m_any));
	if (pPool)
	{
		return pPool->TryRun(task);
	}
	return false;
}

}
