#include <mwnet_mt/base/CountDownLatch.h>
#include <memory>

#include "MWCountDownLatch.h"

typedef std::shared_ptr<mwnet_mt::CountDownLatch> CountDownLatchPtr;

namespace MWCOUNTDOWNLATCH
{
CountDownLatch::CountDownLatch(int nCount)
{
	// ����һ���߳̽�loop�����߳���
	CountDownLatchPtr pCntDownLatch(new mwnet_mt::CountDownLatch(nCount));

	m_any = pCntDownLatch;
}

// ���Ӵ��������ȴ�
void CountDownLatch::AddAndWait(int nAdd)
{
	CountDownLatchPtr pCntDownLatch(boost::any_cast<CountDownLatchPtr>(m_any));

	if (pCntDownLatch)
	{
		pCntDownLatch->addAndWait(nAdd);
	}
}

// �ȵȴ�,�ȵ������Ӵ���
void CountDownLatch::WaitThenAdd(int nAdd)
{
	CountDownLatchPtr pCntDownLatch(boost::any_cast<CountDownLatchPtr>(m_any));

	if (pCntDownLatch)
	{
		pCntDownLatch->waitThenAdd(nAdd);
	}
}

// ���ô��������������д�����0���������
void CountDownLatch::ResetCount(int nCount)
{
	CountDownLatchPtr pCntDownLatch(boost::any_cast<CountDownLatchPtr>(m_any));

	if (pCntDownLatch)
	{
		pCntDownLatch->resetCount(nCount);
	}
}

void CountDownLatch::Wait()
{
	CountDownLatchPtr pCntDownLatch(boost::any_cast<CountDownLatchPtr>(m_any));

	if (pCntDownLatch)
	{
		pCntDownLatch->wait();
	}
}

void CountDownLatch::CountDown()
{
	CountDownLatchPtr pCntDownLatch(boost::any_cast<CountDownLatchPtr>(m_any));

	if (pCntDownLatch)
	{
		pCntDownLatch->countDown();
	}
}

int CountDownLatch::GetCount() const
{
	int nCount = 0;

	CountDownLatchPtr pCntDownLatch(boost::any_cast<CountDownLatchPtr>(m_any));

	if (pCntDownLatch)
	{
		nCount = pCntDownLatch->getCount();
	}

	return nCount;
}
}



