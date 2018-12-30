// Use of this source code is governed by a BSD-style license
// that can be found in the License file.

#ifndef MWNET_MT_BASE_THREADPOOL_H
#define MWNET_MT_BASE_THREADPOOL_H

#include <mwnet_mt/base/Condition.h>
#include <mwnet_mt/base/Mutex.h>
#include <mwnet_mt/base/Thread.h>
#include <mwnet_mt/base/Types.h>

#include <deque>
#include <vector>

namespace mwnet_mt
{

class ThreadPool : noncopyable
{
 public:
  typedef std::function<void ()> Task;

  explicit ThreadPool(const string& nameArg = string("ThreadPool"));
  ~ThreadPool();

  // Must be called before start().
  void setMaxQueueSize(int maxSize) { maxQueueSize_ = maxSize; }
  void setThreadInitCallback(const Task& cb)
  { threadInitCallback_ = cb; }

  void start(int numThreads);
  void stop();

  const string& name() const
  { return name_; }

  size_t queueSize() const;

  // Could block if maxQueueSize > 0
  void run(const Task& f);

  // don't block if full
  bool TryRun(const Task& f);
#ifdef __GXX_EXPERIMENTAL_CXX0X__DONOT_USE
  void run(Task&& f);
  // don't block if full
  bool TryRun(Task&& f);

#endif

 private:
  bool isFull() const;
  void runInThread();
  Task take();

  mutable MutexLock mutex_;
  Condition notEmpty_;
  Condition notFull_;
  string name_;
  Task threadInitCallback_;
  std::vector<std::unique_ptr<mwnet_mt::Thread>> threads_;
  std::deque<Task> queue_;
  size_t maxQueueSize_;
  bool running_;
};

}

#endif // MWNET_MT_BASE_THREADPOOL_H
