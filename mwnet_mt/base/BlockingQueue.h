// Use of this source code is governed by a BSD-style license
// that can be found in the License file.

#ifndef MWNET_MT_BASE_BLOCKINGQUEUE_H
#define MWNET_MT_BASE_BLOCKINGQUEUE_H

#include <mwnet_mt/base/Condition.h>
#include <mwnet_mt/base/Mutex.h>

#include <deque>
#include <assert.h>

namespace mwnet_mt
{

template<typename T>
class BlockingQueue : noncopyable
{
 public:
  BlockingQueue()
    : mutex_(),
      notEmpty_(mutex_),
      queue_()
  {
  }

  void put(const T& x)
  {
    MutexLockGuard lock(mutex_);
    queue_.push_back(x);
    notEmpty_.notify(); // wait morphing saves us
    // http://www.domaigne.com/blog/computing/condvars-signal-with-mutex-locked-or-not/
  }

#ifdef __GXX_EXPERIMENTAL_CXX0X__DONOT_USE
  void put(T&& x)
  {
    MutexLockGuard lock(mutex_);
    queue_.push_back(std::move(x));
    notEmpty_.notify();
  }
  // FIXME: emplace()
#endif

  T take()
  {
    MutexLockGuard lock(mutex_);
    // always use a while-loop, due to spurious wakeup
    while (queue_.empty())
    {
      notEmpty_.wait();
    }
    assert(!queue_.empty());
#ifdef __GXX_EXPERIMENTAL_CXX0X__DONOT_USE
    T front(std::move(queue_.front()));
#else
    T front(queue_.front());
#endif
    queue_.pop_front();
    return front;
  }

  size_t size() const
  {
    MutexLockGuard lock(mutex_);
    return queue_.size();
  }

 private:
  mutable MutexLock mutex_;
  Condition         notEmpty_;
  std::deque<T>     queue_;
};

}

#endif  // MWNET_MT_BASE_BLOCKINGQUEUE_H
