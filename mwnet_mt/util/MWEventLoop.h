#ifndef MW_EVENT_LOOP_H
#define MW_EVENT_LOOP_H

#include <boost/any.hpp>
#include <functional>
#include <string>

namespace MWEVENTLOOP
{
	// 事件响应函数
	typedef std::function<void()> EVENT_FUNCTION;
	
	// 定时器ID
	typedef boost::any TIMER_ID;
	
	class EventLoop
	{
	public:
		explicit EventLoop(const std::string& loopname="MWEventLoop");
		~EventLoop();
	public:
		// 在循环中运行一次
		void RunOnce(const EVENT_FUNCTION& evfunc);

		// 在某一个时间运行一次(暂未实现)
		//void RunAt();

		// 延时一段时间后运行一次
		TIMER_ID RunAfter(double delay, const EVENT_FUNCTION& evfunc);

		// 每隔一段时间运行一次(定时器)
		TIMER_ID RunEvery(double interval, const EVENT_FUNCTION& evfunc);

		// 取消定时
		void CancelTimer(TIMER_ID timerid);
		
		// 退出事件循环
		void QuitLoop();
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

void DealTimeOut(const boost::any& any_params)
{
	printf("deal timeout data....\n");
}

int main()
{
	MWEVENTLOOP::EventLoop evloop;
	//MWEVENTLOOP::EventLoop evloop("myevloop");

	evloop.RunOnce(MyPrint1);

	// RunAfter可以用来替代用线程去轮询检测超时，尤其是网络编程时，可以为每一个发出去的包设一个超时间
	// 在超时间内有回应的可以调用canceltimer取消，对于真正超时的，可以在超时回调中处理超时的数据
	// 只需要执行RunAfter，设置超时时间，然后bind超时回调函数和参数数据(参数用智能指针最佳)
	boost::any any_params;
	MWEVENTLOOP::TIMER_ID tmr_id = evloop.RunAfter(10.0, std::bind(DealTimeOut, any_params));
	//evloop.CancelTimer(tmr_id);
	
	evloop.RunAfter(2.0, MyPrint2);

	evloop.RunEvery(1.0, MyPrint3);
}
*/

