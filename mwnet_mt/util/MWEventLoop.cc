#include "MWEventLoop.h"
#include <mwnet_mt/net/EventLoopThread.h>
#include <mwnet_mt/net/EventLoop.h>
#include <mwnet_mt/base/Thread.h>
#include <mwnet_mt/net/TimerId.h>

#include <stdint.h>
#include <stdio.h>

using namespace mwnet_mt;
using namespace mwnet_mt::net;
using namespace MWEVENTLOOP;

typedef std::shared_ptr<EventLoopThread> EventLoopThreadPtr;

namespace MWEVENTLOOP
{
EventLoop::EventLoop(const std::string& loopname)
{
	// 申请一个线程将loop跑在线程里
	EventLoopThreadPtr pThr(new EventLoopThread(NULL, loopname));
	pThr->startLoop();
	m_any = pThr;
}

EventLoop::~EventLoop()
{
	
}

// 在循环中运行一次
void EventLoop::RunOnce(const EVENT_FUNCTION& evfunc)
{
	EventLoopThreadPtr pThr(boost::any_cast<EventLoopThreadPtr>(m_any));
	if (pThr)
	{
		pThr->getloop()->runInLoop(evfunc);
	}
}
/*
// 在某一个时间运行一次
void CEventLoop::RunAt()
{
	EventLoop* pLoop = reinterpret_cast<EventLoop*>(m_pLoop);
	pLoop->runAt(const Timestamp & time,TimerCallback && cb);
}
*/
// 延时一段时间后运行一次
TIMER_ID EventLoop::RunAfter(double delay, const EVENT_FUNCTION& evfunc)
{
	TimerId timer_id;
	
	EventLoopThreadPtr pThr(boost::any_cast<EventLoopThreadPtr>(m_any));
	if (pThr)
	{
		timer_id = pThr->getloop()->runAfter(delay, evfunc);
	}

	boost::any _any = timer_id;
	
	return _any;
}

// 每隔一段时间运行一次(定时器)
TIMER_ID EventLoop::RunEvery(double interval, const EVENT_FUNCTION& evfunc)
{
	TimerId timer_id;
	
	EventLoopThreadPtr pThr(boost::any_cast<EventLoopThreadPtr>(m_any));
	if (pThr)
	{
		timer_id = pThr->getloop()->runEvery(interval, evfunc);
	}

	boost::any _any = timer_id;
	
	return _any;
}

// 取消定时
void EventLoop::CancelTimer(TIMER_ID timerid)
{
	EventLoopThreadPtr pThr(boost::any_cast<EventLoopThreadPtr>(m_any));
	if (pThr)
	{
		pThr->getloop()->cancel(boost::any_cast<TimerId>(timerid));
	}
}

// 退出事件循环
void EventLoop::QuitLoop()
{
	EventLoopThreadPtr pThr(boost::any_cast<EventLoopThreadPtr>(m_any));
	if (pThr)
	{
		pThr->getloop()->runInLoop(std::bind(&mwnet_mt::net::EventLoop::quit, pThr->getloop()));
	}	
}

}

