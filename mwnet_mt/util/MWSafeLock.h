#ifndef MW_SAFELOCK_H
#define MW_SAFELOCK_H

#include <boost/any.hpp>

namespace MWSAFELOCK
{
	class BaseLock
	{
	public:
		BaseLock();
		~BaseLock();	
	public:
		void Lock();
		void UnLock();
	private:
		boost::any m_any;
	};

	class SafeLock
	{
	public:
  		explicit SafeLock(BaseLock& lock); // suggested
			explicit SafeLock(BaseLock* plock); // just compatible with old bussiness system
  		~SafeLock();
	private:
  		BaseLock* m_pLock;
	};
}
#endif 

/*
调用示例:

using namespace MWSAFELOCK;

//必须定义成类成员变量或全局变量,不要再定义成指针了,内存已有智能指针
MWSAFELOCK::BaseLock lock; 
int main(int argc, char* argv[])
{
	{
		// lock....
		
		//必须定义成临时变量,每天需要锁定时,以本行类似的形态存在,临时变量析构时会自动解锁
		MWSAFELOCK::SafeLock safelock(lock); 
		//MWSAFELOCK::SafeLock safelock(&lock); 
		
		// do something......

		// no need unlock manually
	}

	// when safelock leave scope,auto destruct,auto unlock
		
	while(1)
	{
		sleep(1);
	}
}
*/
