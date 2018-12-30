// Use of this source code is governed by a BSD-style license
// that can be found in the License file.

// This is a public header file, it must only include public header files.

#ifndef MWNET_MT_NET_EVENTLOOPTHREAD_H
#define MWNET_MT_NET_EVENTLOOPTHREAD_H

#include <mwnet_mt/base/Condition.h>
#include <mwnet_mt/base/Mutex.h>
#include <mwnet_mt/base/Thread.h>

namespace mwnet_mt
{
namespace net
{

class EventLoop;

class EventLoopThread : noncopyable
{
 public:
  typedef std::function<void(EventLoop*)> ThreadInitCallback;

  EventLoopThread(const ThreadInitCallback& cb = ThreadInitCallback(),
                  const string& name = string());
  ~EventLoopThread();
  EventLoop* startLoop();
  EventLoop* getloop();
  
 private:
  void threadFunc();

  EventLoop* loop_;
  bool exiting_;
  Thread thread_;
  MutexLock mutex_;
  Condition cond_;
  ThreadInitCallback callback_;
};

}
}

#endif  // MWNET_MT_NET_EVENTLOOPTHREAD_H

