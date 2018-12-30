#include <mwnet_mt/base/CountDownLatch.h>
#include <memory>

#include "MWCountDownLatch.h"

typedef std::shared_ptr<mwnet_mt::CountDownLatch> CountDownLatchPtr;

namespace MWCOUNTDOWNLATCH
{
CountDownLatch::CountDownLatch(int nCount)
{
	// 申请一个线程将loop跑在线程里
	CountDownLatchPtr pCntDownLatch(new mwnet_mt::CountDownLatch(nCount));

	m_any = pCntDownLatch;
}

// 增加次数，并等待
void CountDownLatch::AddAndWait(int nAdd)
{
	CountDownLatchPtr pCntDownLatch(boost::any_cast<CountDownLatchPtr>(m_any));

	if (pCntDownLatch)
	{
		pCntDownLatch->addAndWait(nAdd);
	}
}

// 重置次数，仅限于所有次数归0后才能重置
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



