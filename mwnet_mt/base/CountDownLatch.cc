// Use of this source code is governed by a BSD-style license
// that can be found in the License file.

#include <mwnet_mt/base/CountDownLatch.h>

using namespace mwnet_mt;

CountDownLatch::CountDownLatch(int count)
  : mutex_(),
    condition_(mutex_),
    count_(count)
{
}

void CountDownLatch::wait()
{
	MutexLockGuard lock(mutex_);
	while (count_ > 0)
	{
		condition_.wait();
	}
}

void CountDownLatch::addAndWait(int count)
{
	MutexLockGuard lock(mutex_);

	count_ += count;
	
	while (count_ > 0)
	{
	  condition_.wait();
	}
}

void CountDownLatch::waitThenAdd(int count)
{
	MutexLockGuard lock(mutex_);

	while (count_ > 0)
	{
		condition_.wait();
	}

	count_ += count;
}

void CountDownLatch::resetCount(int count)
{
	count_ = count;
}

void CountDownLatch::countDown()
{
	MutexLockGuard lock(mutex_);
	--count_;
	if (count_ == 0)
	{
		condition_.notifyAll();
	}
}

int CountDownLatch::getCount() const
{
	MutexLockGuard lock(mutex_);
	return count_;
}

