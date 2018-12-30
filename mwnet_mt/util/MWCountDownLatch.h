#ifndef MW_COUNTDOWNLATCH_H
#define MW_COUNTDOWNLATCH_H

#include <boost/any.hpp>

namespace MWCOUNTDOWNLATCH
{
class CountDownLatch
{
public:
	// nCount：CountDown所需要的次数
	explicit CountDownLatch(int nCount);

	// 增加次数，并等待
	void AddAndWait(int nAdd);

	// 重置次数，仅限于所有次数归0后才能重置
	void ResetCount(int nCount);

	// 等待
	void Wait();

	// 减少计数
	void CountDown();

	// 获取当前计数
	int GetCount() const;

private:
	boost::any m_any;
};
}
#endif

// 用法示例
/* 
#include <iostream>
#include <thread>

#include "../MWCountDownLatch.h"

#define THREAD_NUM 5

using namespace std;
using namespace MWCOUNTDOWNLATCH;

CountDownLatch countDownLatch(THREAD_NUM);

void func_test()
{
cout << "thread start id:" << this_thread::get_id() << endl;
//计数-1
countDownLatch.CountDown();
}

int main()
{
cout << "thread main id:" << this_thread::get_id() << endl;

for (int i = 0; i < THREAD_NUM; ++i)
{
thread t(func_test);
t.detach();
}

cout << "thread create End" << endl;
//等待所有的线程起来
countDownLatch.Wait();
cout << "thread start End" << endl;

while (1);
return 0;
}
*/