// Use of this source code is governed by a BSD-style license
// that can be found in the License file.

#include <mwnet_mt/net/EventLoopThreadPool.h>

#include <mwnet_mt/net/EventLoop.h>
#include <mwnet_mt/net/EventLoopThread.h>

#include <stdio.h>

using namespace mwnet_mt;
using namespace mwnet_mt::net;

EventLoopThreadPool::EventLoopThreadPool(EventLoop* baseLoop, const string& nameArg)
  : baseLoop_(baseLoop),
    name_(nameArg),
    started_(false),
    numThreads_(0),
    next_(0)
{
}

EventLoopThreadPool::~EventLoopThreadPool()
{
  // Don't delete loop, it's stack variable

	for (auto& item : loops_)
	{
		item->quit();
	}
	loops_.clear();

	for (auto& item : threads_)
	{
		item.reset();
	}
	threads_.clear();
}

void EventLoopThreadPool::start(const ThreadInitCallback& cb)
{
  assert(!started_);
  baseLoop_->assertInLoopThread();

  started_ = true;

  for (int i = 0; i < numThreads_; ++i)
  {
    char buf[name_.size() + 32];
    snprintf(buf, sizeof buf, "%s_%02d", name_.c_str(), i+1);
    EventLoopThread* t = new EventLoopThread(cb, buf);
    threads_.push_back(std::unique_ptr<EventLoopThread>(t));
	EventLoop* p = t->startLoop();
	p->setidinpool(i+1);
    loops_.push_back(p);
  }
  if (numThreads_ == 0 && cb)
  {
    cb(baseLoop_);
  }
}

EventLoop* EventLoopThreadPool::getNextLoop()
{
  baseLoop_->assertInLoopThread();
  assert(started_);
  EventLoop* loop = baseLoop_;

  if (!loops_.empty())
  {
    // round-robin
    loop = loops_[next_];
    ++next_;
    if (implicit_cast<size_t>(next_) >= loops_.size())
    {
      next_ = 0;
    }
  }
  return loop;
}

EventLoop* EventLoopThreadPool::getLoopForHash(size_t hashCode)
{
  baseLoop_->assertInLoopThread();
  EventLoop* loop = baseLoop_;

  if (!loops_.empty())
  {
    loop = loops_[hashCode % loops_.size()];
  }
  return loop;
}

std::vector<EventLoop*> EventLoopThreadPool::getAllLoops()
{
  baseLoop_->assertInLoopThread();
  assert(started_);
  if (loops_.empty())
  {
    return std::vector<EventLoop*>(1, baseLoop_);
  }
  else
  {
    return loops_;
  }
}
