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