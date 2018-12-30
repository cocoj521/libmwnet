#include <mwnet_mt/net/EventLoop.h>
#include <mwnet_mt/base/Thread.h>

#include <assert.h>
#include <stdio.h>
#include <unistd.h>

using namespace mwnet_mt;
using namespace mwnet_mt::net;

EventLoop* g_loop;

void callback()
{
  printf("callback(): pid = %d, tid = %d\n", getpid(), CurrentThread::tid());
  EventLoop anotherLoop;
}

void threadFunc()
{
  printf("threadFunc(): pid = %d, tid = %d\n", getpid(), CurrentThread::tid());

  assert(EventLoop::getEventLoopOfCurrentThread() == NULL);
  EventLoop loop;
  assert(EventLoop::getEventLoopOfCurrentThread() == &loop);
  loop.runAfter(1.0, callback);
  loop.loop();
}

int main()
{
  printf("main(): pid = %d, tid = %d\n", getpid(), CurrentThread::tid());

  assert(EventLoop::getEventLoopOfCurrentThread() == NULL);
  EventLoop loop;
  assert(EventLoop::getEventLoopOfCurrentThread() == &loop);

  Thread thread(threadFunc);
  thread.start();

  loop.loop();
}
