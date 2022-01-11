#ifndef MW_COUNTDOWNLATCH_H
#define MW_COUNTDOWNLATCH_H

#include <boost/any.hpp>

namespace MWCOUNTDOWNLATCH
{
class CountDownLatch
{
public:
	// nCount��CountDown����Ҫ�Ĵ���
	explicit CountDownLatch(int nCount);

	// ���Ӵ��������ȴ�
	void AddAndWait(int nAdd);

	// �ȵȴ�,�ȵ������Ӵ���
	void WaitThenAdd(int nAdd);

	// ���ô��������������д�����0���������
	void ResetCount(int nCount);

	// �ȴ�
	void Wait();

	// ���ټ���
	void CountDown();

	// ��ȡ��ǰ����
	int GetCount() const;

private:
	boost::any m_any;
};
}
#endif

// �÷�ʾ��
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
//����-1
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
//�ȴ����е��߳�����
countDownLatch.Wait();
cout << "thread start End" << endl;

while (1);
return 0;
}
*/