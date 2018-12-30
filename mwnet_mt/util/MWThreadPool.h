#ifndef MW_THREADPOOL_H
#define MW_THREADPOOL_H

#include <boost/any.hpp>
#include <functional>
#include <string>

namespace MWTHREADPOOL
{
	// 线程池执行的任务函数
	typedef std::function<void ()> THREAD_TASK;
	
	class ThreadPool
	{
	public:
		explicit ThreadPool(const std::string& poolname);
		~ThreadPool();
	public:
		// Must be called before StartThreadPool().
		// 设置任务队列最大值,任务队列超过该值时,再次添加会阻塞等待
		// maxquesize<=0表示不限制大小
		void SetMaxTaskQueueSize(int maxquesize);

		// Must be called before StartThreadPool().
		// 设置线程初始化时执行的回调函数.如果有什么需要在线程启动前要执行的,可以设置该回调函数并实现
  		void SetThreadInitCallback(const THREAD_TASK& thrinitcb);

		// 在线程池中启动一定数量的线程
		// thrnum>=0
		void StartThreadPool(int thrnum);

		// 停止线程池
  		void StopThreadPool();

		// 线程池名字
  		const char* TheadPoolName();

		// 当前任务队列的大小
  		size_t TaskQueueSize() const;

  		// 执行任务.如果任务队列超过最大值时,该函数会阻塞
  		void RunTask(const THREAD_TASK& task);

  		// 尝试执行任务.如果任务队列超过最大值时,该函数不会阻塞,会返回false.
  		bool TryRunTask(const THREAD_TASK& task);
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
		MWTHREADPOOL::ThreadPool thrpool("test");
		
		thrpool.SetMaxTaskQueueSize(100);
		thrpool.SetThreadInitCallback(MyPrint1);
		thrpool.StartThreadPool(2);
		thrpool.TryRunTask(MyPrint2);
		thrpool.StopThreadPool();
	}

	{
		MWTHREADPOOL::ThreadPool thrpool("test");
		
		thrpool.SetMaxTaskQueueSize(100);
		thrpool.SetThreadInitCallback(std::bind(MyPrint4,400));
		thrpool.StartThreadPool(2);
		thrpool.TryRunTask(MyPrint1);
		thrpool.TryRunTask(std::bind(MyPrint4,400));
		thrpool.StopThreadPool();
	}

	//{
		std::unique_ptr<MWTHREADPOOL::ThreadPool> pThrPool(new MWTHREADPOOL::ThreadPool("test"));
	
		pThrPool->SetMaxTaskQueueSize(100);
		pThrPool->SetThreadInitCallback(std::bind(MyPrint4,400));
		pThrPool->StartThreadPool(2);
		pThrPool->TryRunTask(MyPrint3);
		pThrPool->TryRunTask(std::bind(MyPrint4,400));
		printf("%s\n", pThrPool->TheadPoolName());
		pThrPool->StopThreadPool();
	//}
		
	while(1)
	{
		sleep(1);
	}
}
*/
