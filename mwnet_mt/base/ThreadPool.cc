// Use of this source code is governed by a BSD-style license
// that can be found in the License file.

#include <mwnet_mt/base/ThreadPool.h>
#include <mwnet_mt/base/Logging.h>
#include <mwnet_mt/base/Exception.h>

#include <assert.h>
#include <stdio.h>

using namespace mwnet_mt;

ThreadPool::ThreadPool(const string& nameArg)
  : mutex_(),
    notEmpty_(mutex_),
    notFull_(mutex_),
    name_(nameArg),
    maxQueueSize_(0),
    running_(false)
{
}

ThreadPool::~ThreadPool()
{
  if (running_)
  {
    stop();
  }
}

void ThreadPool::start(int numThreads)
{
  assert(threads_.empty());
  running_ = true;
  threads_.reserve(numThreads);
  for (int i = 0; i < numThreads; ++i)
  {
    char id[32];
    snprintf(id, sizeof id, "%02d", i+1);
    threads_.emplace_back(new mwnet_mt::Thread(
          std::bind(&ThreadPool::runInThread, this), name_+id));
    threads_[i]->start();
  }
  if (numThreads == 0 && threadInitCallback_)
  {
    threadInitCallback_();
  }
}

void ThreadPool::stop()
{
  {
  MutexLockGuard lock(mutex_);
  running_ = false;
  notEmpty_.notifyAll();
  }
  for (auto& thr : threads_)
  {
    thr->join();
  }
}

size_t ThreadPool::queueSize() const
{
  MutexLockGuard lock(mutex_);
  return queue_.size();
}

void ThreadPool::run(const Task& task)
{
  if (threads_.empty())
  {
    task();
  }
  else
  {
    MutexLockGuard lock(mutex_);
    while (isFull())
    {
      notFull_.wait();
    }
    assert(!isFull());

    queue_.push_back(task);
    notEmpty_.notify();
  }
}

bool ThreadPool::TryRun(const Task& task)
{
  if (threads_.empty())
  {
    task();
  }
  else
  {
    MutexLockGuard lock(mutex_);
    if (isFull())
    {
      return false;
    }
    assert(!isFull());

    queue_.push_back(task);
    notEmpty_.notify();
  }
  return true;
}

#ifdef __GXX_EXPERIMENTAL_CXX0X__DONOT_USE
void ThreadPool::run(Task&& task)
{
  if (threads_.empty())
  {
    task();
  }
  else
  {
    MutexLockGuard lock(mutex_);
    while (isFull())
    {
      notFull_.wait();
    }
    assert(!isFull());

    queue_.push_back(std::move(task));
    notEmpty_.notify();
  }
}

bool ThreadPool::TryRun(Task&& task)
{
  if (threads_.empty())
  {
    task();
  }
  else
  {
    MutexLockGuard lock(mutex_);
    if (isFull())
    {
      return false;
    }
    assert(!isFull());

    queue_.push_back(std::move(task));
    notEmpty_.notify();
  }
  return true;
}

#endif

ThreadPool::Task ThreadPool::take()
{
  MutexLockGuard lock(mutex_);
  // always use a while-loop, due to spurious wakeup
  while (queue_.empty() && running_)
  {
    notEmpty_.wait();
  }
  Task task;
  if (!queue_.empty())
  {
    task = queue_.front();
    queue_.pop_front();
    if (maxQueueSize_ > 0)
    {
      notFull_.notify();
    }
  }
  return task;
}

bool ThreadPool::isFull() const
{
  mutex_.assertLocked();
  return maxQueueSize_ > 0 && queue_.size() >= maxQueueSize_;
}

void ThreadPool::runInThread()
{
  try
  {
    if (threadInitCallback_)
    {
      threadInitCallback_();
    }
    while (running_)
    {
      Task task(take());
      if (task)
      {
        task();
      }
    }
  }
  catch (const Exception& ex)
  {
	fprintf(stderr, "exception caught in ThreadPool %s\n", name_.c_str());
	fprintf(stderr, "reason: %s\n", ex.what());
	fprintf(stderr, "stack trace: %s\n", ex.stackTrace());
	LOG_SYSERR << "exception caught in ThreadPool:" << name_ << "\n" 
			   << "reason: " << ex.what() << "\n"
			   << "stack trace: " << ex.stackTrace() << "\n" ;
    //abort();
  }
  catch (const std::exception& ex)
  {
	fprintf(stderr, "exception caught in ThreadPool %s\n", name_.c_str());
	fprintf(stderr, "reason: %s\n", ex.what());
	LOG_SYSERR << "exception caught in ThreadPool:" << name_ << "\n" 
			   << "reason: " << ex.what() << "\n";
    //abort();
  }
  catch (...)
  {
    fprintf(stderr, "unknown exception caught in ThreadPool %s\n", name_.c_str());
	LOG_SYSERR << "unknown exception caught in ThreadPool:" << name_ << "\n"; 
    //throw; // rethrow
  }
}

