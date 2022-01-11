// Use of this source code is governed by a BSD-style license
// that can be found in the License file.

#ifndef MWNET_MT_BASE_COUNTDOWNLATCH_H
#define MWNET_MT_BASE_COUNTDOWNLATCH_H

#include <mwnet_mt/base/Condition.h>
#include <mwnet_mt/base/Mutex.h>

namespace mwnet_mt
{

class CountDownLatch : noncopyable
{
 public:

  explicit CountDownLatch(int count);

  void wait();

  void addAndWait(int count);

  void waitThenAdd(int count);

  // 仅当计数归0后才能重设
  void resetCount(int count);
  
  void countDown();

  int getCount() const;

 private:
  mutable MutexLock mutex_;
  Condition condition_;
  int count_;
};

}
#endif  // MWNET_MT_BASE_COUNTDOWNLATCH_H
