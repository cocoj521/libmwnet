#ifndef MWNET_MT_BASE_ASYNCLOGGING_H
#define MWNET_MT_BASE_ASYNCLOGGING_H

#include <mwnet_mt/base/BlockingQueue.h>
#include <mwnet_mt/base/BoundedBlockingQueue.h>
#include <mwnet_mt/base/CountDownLatch.h>
#include <mwnet_mt/base/Mutex.h>
#include <mwnet_mt/base/Thread.h>
#include <mwnet_mt/base/LogStream.h>

#include <vector>

namespace mwnet_mt
{

class AsyncLogging : noncopyable
{
 public:

  AsyncLogging(const string& basepath,
               const string& basename,
               size_t rollSize,
               int flushInterval = 3);

  ~AsyncLogging()
  {
    if (running_)
    {
      stop();
    }
  }

  void append(const char* logline, int len);

  void start()
  {
    running_ = true;
    thread_.start();
    latch_.wait();
  }

  void stop()
  {
    running_ = false;
    cond_.notify();
    thread_.join();
  }

 private:

  void threadFunc();

  typedef mwnet_mt::detail::FixedBuffer<mwnet_mt::detail::kLargeBuffer> Buffer;
  typedef std::vector<std::unique_ptr<Buffer>> BufferVector;
  typedef BufferVector::value_type BufferPtr;

  const int flushInterval_;
  bool running_;
  string basepath_;
  string basename_;
  size_t rollSize_;
  mwnet_mt::Thread thread_;
  mwnet_mt::CountDownLatch latch_;
  mwnet_mt::MutexLock mutex_;
  mwnet_mt::Condition cond_;
  BufferPtr currentBuffer_;
  BufferPtr nextBuffer_;
  BufferVector buffers_;
};

}
#endif  // MWNET_MT_BASE_ASYNCLOGGING_H
