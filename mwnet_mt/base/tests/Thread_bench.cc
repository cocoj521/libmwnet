#include <mwnet_mt/base/CurrentThread.h>
#include <mwnet_mt/base/Mutex.h>
#include <mwnet_mt/base/Thread.h>
#include <mwnet_mt/base/Timestamp.h>

#include <map>
#include <string>
#include <stdio.h>
#include <sys/wait.h>

mwnet_mt::MutexLock g_mutex;
std::map<int, int> g_delays;

void threadFunc()
{
  //printf("tid=%d\n", mwnet_mt::CurrentThread::tid());
}

void threadFunc2(mwnet_mt::Timestamp start)
{
  mwnet_mt::Timestamp now(mwnet_mt::Timestamp::now());
  int delay = static_cast<int>(timeDifference(now, start) * 1000000);
  mwnet_mt::MutexLockGuard lock(g_mutex);
  ++g_delays[delay];
}

void forkBench()
{
  sleep(10);
  mwnet_mt::Timestamp start(mwnet_mt::Timestamp::now());
  int kProcesses = 10*1000;

  for (int i = 0; i < kProcesses; ++i)
  {
    pid_t child = fork();
    if (child == 0)
    {
      exit(0);
    }
    else
    {
      waitpid(child, NULL, 0);
    }
  }

  double timeUsed = timeDifference(mwnet_mt::Timestamp::now(), start);
  printf("process creation time used %f us\n", timeUsed*1000000/kProcesses);
  printf("number of created processes %d\n", kProcesses);
}

int main(int argc, char* argv[])
{
  printf("pid=%d, tid=%d\n", ::getpid(), mwnet_mt::CurrentThread::tid());
  mwnet_mt::Timestamp start(mwnet_mt::Timestamp::now());

  int kThreads = 100*1000;
  for (int i = 0; i < kThreads; ++i)
  {
    mwnet_mt::Thread t1(threadFunc);
    t1.start();
    t1.join();
  }

  double timeUsed = timeDifference(mwnet_mt::Timestamp::now(), start);
  printf("thread creation time %f us\n", timeUsed*1000000/kThreads);
  printf("number of created threads %d\n", mwnet_mt::Thread::numCreated());

  for (int i = 0; i < kThreads; ++i)
  {
    mwnet_mt::Timestamp now(mwnet_mt::Timestamp::now());
    mwnet_mt::Thread t2(std::bind(threadFunc2, now));
    t2.start();
    t2.join();
  }
  {
    mwnet_mt::MutexLockGuard lock(g_mutex);
    for (std::map<int, int>::iterator it = g_delays.begin();
        it != g_delays.end(); ++it)
    {
      printf("delay = %d, count = %d\n",
             it->first, it->second);
    }
  }

  forkBench();
}
