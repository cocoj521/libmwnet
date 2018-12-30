#include <mwnet_mt/base/BlockingQueue.h>
#include <mwnet_mt/base/CountDownLatch.h>
#include <mwnet_mt/base/Thread.h>
#include <mwnet_mt/base/Timestamp.h>

#include <map>
#include <string>
#include <vector>
#include <stdio.h>

using std::placeholders::_1;

class Bench
{
 public:
  Bench(int numThreads)
    : latch_(numThreads),
      threads_(numThreads)
  {
    for (int i = 0; i < numThreads; ++i)
    {
      char name[32];
      snprintf(name, sizeof name, "work thread %d", i);
      threads_.emplace_back(new mwnet_mt::Thread(
            std::bind(&Bench::threadFunc, this), mwnet_mt::string(name)));
    }
    for (auto& thr : threads_)
    {
      thr->start();
    }
  }

  void run(int times)
  {
    printf("waiting for count down latch\n");
    latch_.wait();
    printf("all threads started\n");
    for (int i = 0; i < times; ++i)
    {
      mwnet_mt::Timestamp now(mwnet_mt::Timestamp::now());
      queue_.put(now);
      usleep(1000);
    }
  }

  void joinAll()
  {
    for (size_t i = 0; i < threads_.size(); ++i)
    {
      queue_.put(mwnet_mt::Timestamp::invalid());
    }

    for (auto& thr : threads_)
    {
      thr->join();
    }
  }

 private:

  void threadFunc()
  {
    printf("tid=%d, %s started\n",
           mwnet_mt::CurrentThread::tid(),
           mwnet_mt::CurrentThread::name());

    std::map<int, int> delays;
    latch_.countDown();
    bool running = true;
    while (running)
    {
      mwnet_mt::Timestamp t(queue_.take());
      mwnet_mt::Timestamp now(mwnet_mt::Timestamp::now());
      if (t.valid())
      {
        int delay = static_cast<int>(timeDifference(now, t) * 1000000);
        // printf("tid=%d, latency = %d us\n",
        //        mwnet_mt::CurrentThread::tid(), delay);
        ++delays[delay];
      }
      running = t.valid();
    }

    printf("tid=%d, %s stopped\n",
           mwnet_mt::CurrentThread::tid(),
           mwnet_mt::CurrentThread::name());
    for (std::map<int, int>::iterator it = delays.begin();
        it != delays.end(); ++it)
    {
      printf("tid = %d, delay = %d, count = %d\n",
             mwnet_mt::CurrentThread::tid(),
             it->first, it->second);
    }
  }

  mwnet_mt::BlockingQueue<mwnet_mt::Timestamp> queue_;
  mwnet_mt::CountDownLatch latch_;
  std::vector<std::unique_ptr<mwnet_mt::Thread>> threads_;
};

int main(int argc, char* argv[])
{
  int threads = argc > 1 ? atoi(argv[1]) : 1;

  Bench t(threads);
  t.run(10000);
  t.joinAll();
}
