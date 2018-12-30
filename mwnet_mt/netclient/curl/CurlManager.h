#ifndef __CURL_CURLMANAGER_H__
#define __CURL_CURLMANAGER_H__

#include "CurlRequest.h"
#include "CurlEvMgr.h"
#include <mwnet_mt/net/TimerId.h>
#include <mutex>
#include <map>
#include <list>

using namespace mwnet_mt;
using namespace mwnet_mt::net;
namespace curl
{

/**
 * curlm句柄的管理者，可管理多个简单句柄
 */
class CurlManager : noncopyable
{
public:
	enum Option
	{
		kCURLnossl = 0,
		kCURLssl   = 1,
	};

	/**
	 * [CurlManager 构造函数]
	 * @param  loop [epoll的事件循环器]
	 */
	explicit CurlManager(EventLoop* loop);
	~CurlManager();

	/**
	 * [initialize 初始化函数]
	 * @param opt [kCURLnossl：不支持HTTPS kCURLssl：支持HTTPS]
	 */
	static void initialize(Option opt = kCURLnossl);
	static void uninitialize();
	
	// 获取一个请求对象
	static CurlRequestPtr getRequest(const std::string& url, bool bKeepAlive, int req_type, int http_ver);

	// 发送请求
	//0:成功 1:队列满
	int sendRequest(const CurlRequestPtr& request);

	/**
	 * [读事件的响应函数]
	 * @param fd [事件的套接口]
	 */
	void onReadEventCallBack(int fd);
	
	/**
	 * [写事件的响应函数]
	 * @param fd [事件的套接口]
	 */
	void onWriteEventCallBack(int fd);

private:
	// 通知发送
	void notifySendRequest();
	void onRecvWakeUpNotify();
	
	/**
	 * [check_multi_info 核实请求是否已经完成]
	 */
	void check_multi_info();

	// 检查是否还有需要发送的请求
	void checkNeedSendRequest();

	// 回收请求,视情况而定是回收重用还是直接释放
	void recycleRequest(const CurlRequestPtr& request);

	// 发送请求	
	void sendRequestInLoop(const CurlRequestPtr& request);

	// 将请求移出管理器
	void removeMultiHandle(const CurlRequestPtr& request);

	void afterRequestDone(const CurlRequestPtr& request);
	
	// curlm socket事件回调CURLMOPT_SOCKETFUNCTION CURL_POLL_IN/OUT/INOUT/REMOVE
	static int curlmSocketOptCb(CURL* c, int fd, int what, void* userp, void* socketp);
	int curlmSocketOptCbInLoop(CURL* c, int fd, int what, void* socketp);
	void addEvLoop(void* p, int fd, int what);
	void optEvLoop(void* p, int fd, int what);
	void delEvLoop(void* p, int fd, int what);
	
	// curlm超时回调函数CURLMOPT_TIMERFUNCTION
	static int curlmTimerCb(CURLM* curlm, long ms, void* userp);
	void curlmTimerCbInLoop(long ms);
	void createTimer(long ms);
	void cancelTimer();
	
private:
	// 事件循环器
	EventLoop* loop_;
	// 事件管理器
	std::shared_ptr<CurlEvMgr> curl_evmgr_;
	// 唤醒FD
	int wakeupFd_;
	// 唤醒通道
	std::shared_ptr<Channel> wakeupChannel_;
	// 多路句柄管理器
	CURLM* curlm_;
	// 记录正在被监控的简单句柄的总个数
	int runningHandles_;
	// 记录上一次监控的简单句柄的总个数
	int prevRunningHandles_;
	// 定时器ID
	TimerId timerid_;
};

} // end namespace curl


#endif // __CURL_CURLMANAGER_H__
