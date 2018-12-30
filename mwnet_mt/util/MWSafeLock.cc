#include "MWSafeLock.h"
#include <mwnet_mt/base/Mutex.h>
#include <mwnet_mt/base/Thread.h>

using namespace mwnet_mt;
using namespace MWSAFELOCK;

typedef std::shared_ptr<mwnet_mt::MutexLock> MutexLockPtr;

namespace MWSAFELOCK
{
BaseLock::BaseLock()
{
	MutexLockPtr pMutexLock(new mwnet_mt::MutexLock());
	m_any = pMutexLock;
}

BaseLock::~BaseLock()
{
	
}

void BaseLock::Lock()
{
	MutexLockPtr pMutexLock(boost::any_cast<MutexLockPtr>(m_any));
	if (pMutexLock)
	{
		pMutexLock->lock();
	}
}

void BaseLock::UnLock()
{
	MutexLockPtr pMutexLock(boost::any_cast<MutexLockPtr>(m_any));
	if (pMutexLock)
	{
		pMutexLock->unlock();
	}
}

SafeLock::SafeLock(BaseLock& lock)
{
	m_pLock = &lock;
	if (NULL != m_pLock)
	{
		m_pLock->Lock();
	}
}

SafeLock::SafeLock(BaseLock* pLock)
{
	m_pLock = pLock;
	if (NULL != m_pLock)
	{
		m_pLock->Lock();
	}
}

SafeLock::~SafeLock()
{
	if (NULL != m_pLock)
	{
		m_pLock->UnLock();
	}
}

}
