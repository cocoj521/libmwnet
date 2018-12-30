#ifndef MW_THREAD_H
#define MW_THREAD_H

#include <boost/any.hpp>
#include <functional>
#include <string>

namespace MWTHREAD
{
	// 线程函数
	typedef std::function<void ()> THREAD_FUNC;
	
	class Thread
	{
	public:
		explicit Thread(const THREAD_FUNC& thrfunc, const std::string& thrname);
		~Thread();
	public:
		// 启动成功返回线程ID,启动失败返回0
		pid_t StartThread();

		// return pthread_join()
		// 0:success other:fail
		int   JoinThread(); 

		// 返回线程名称
		const char* ThreadName();
	private:
		boost::any m_any;
	};
}
#endif

/*
调用示例:

void MyPrint1()
{
	printf("myprint1....\n");
}

void MyPrint2()
{
	printf("myprint2....\n");
}

void MyPrint3()
{
	printf("myprint3....\n");
}
void MyPrint4(int n)
{
	printf("myprint4....%d\n", n);
}

int main(int argc, char* argv[])
{
	{
		MWTHREAD::Thread thr(MyPrint1, "test");
		
		pid_t pd = thr.StartThread();

		int ret = -1;
		
		printf("test thread:%d,%d\n", pd, ret);

		ret = thr.JoinThread();
		
		printf("test thread:%d,%d\n", pd, ret);
	}


	{
		MWTHREAD::Thread thr(std::bind(MyPrint4, 10), "test");
		
		pid_t pd = thr.StartThread();

		int ret = -1;
		
		printf("test thread:%d,%d\n", pd, ret);

		ret = thr.JoinThread();
		
		printf("test thread:%d,%d\n", pd, ret);
	}

	{
		std::unique_ptr<MWTHREAD::Thread> pThr(new MWTHREAD::Thread(std::bind(MyPrint4, 10), "test"));
	
		pid_t pd = pThr->StartThread();

		int ret = -1;
		
		printf("test thread:%d,%d\n", pd, ret);

		ret = pThr->JoinThread();
		
		printf("test thread:%d,%d\n", pd, ret);
	}
		
	while(1)
	{
		sleep(1);
	}
}

*/
