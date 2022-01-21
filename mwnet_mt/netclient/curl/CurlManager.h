#ifndef __CURL_CURLMANAGER_H__
#define __CURL_CURLMANAGER_H__

#include "CurlRequest.h"
#include <mwnet_mt/base/CountDownLatch.h>
#include <mutex>
#include <map>
#include <list>
#include <event2/event.h>
#include <event2/event_struct.h>

namespace curl
{

typedef struct _EvLoopInfo
{
	struct event_base* evbase;
	struct event wakeup_event;
	struct event timer_event;
	//struct event request_timeout_event; //todo:超时检测
} EvLoopInfo;

/* Information associated with a specific socket */
typedef struct _SockInfo
{
	int fd;
	CURL* easy;
	int action;
	long timeout;
	struct event ev;
	EvLoopInfo *evInfo;
} SockInfo;

/**
 * curlm句柄的管理者，可管理多个简单句柄
 */
class CurlManager : noncopyable,
					public std::enable_shared_from_this<CurlManager>
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
	explicit CurlManager();
	~CurlManager();

	/**
	 * [initialize 初始化函数]
	 * @param opt [kCURLnossl：不支持HTTPS kCURLssl：支持HTTPS]
	 */
	static void initialize(Option opt = kCURLnossl);
	static void uninitialize();
	
	// 获取一个请求对象
	static CurlRequestPtr getRequest(const std::string& url, bool bKeepAlive, int req_type, int http_ver);

	// 启动curl事件循环（启动事件循环等，该函数阻塞，不会返回，需要放在线程中运行）
	int runCurlEvLoop(const std::shared_ptr<CountDownLatch>& latch);
	
	// 发送请求
	//0:成功 1:尚未初始化 2:发送缓冲满 3:超过总的最大并发数
	int sendRequest(const CurlRequestPtr& request);

	// 处理中断请求
	void forceCancelRequest(uint64_t req_uuid);
	// 处理中断请求(内部调用)
	void forceCancelRequestInner(const CurlRequestPtr& request);

	// wakeup
	static void onRecvWakeUpNotify(int fd, short evtp, void *arg);
	void handleWakeUpCb();

	// evtimer
	static void onRecvEvTimerNotify(int fd, short evtp, void *arg);
	void handleEvTimerCb();

	// ev事件操作通知
	static void onRecvEvOptNotify(int fd, short evtp, void *arg);
	void handleEvOptCb(int fd, short evtp);

	// request请求超时事件操作通知
	static void onRequestTimeOutNotify(int fd, short evtp, void *arg);
	void handleRequestTimeCb(int fd, short evtp);

	// curlm socket事件回调CURLMOPT_SOCKETFUNCTION CURL_POLL_IN/OUT/INOUT/REMOVE
	static int curlmSocketOptCb(CURL* c, int fd, int what, void* userp, void* socketp);
	void handleCurlmSocketOptCb(CURL* c, int fd, int what, void* socketp);

	// curlm超时回调函数CURLMOPT_TIMERFUNCTION
	static int curlmTimerCb(CURLM* curlm, long ms, void* userp);
	void handleCurlmTimerCb(long ms);

	void addsock(int fd, CURL* c, int action, EvLoopInfo* evInfo);

	/* Assign information to a SockInfo structure */
	void setsock(SockInfo* sinfo, int fd, CURL* c, int action, EvLoopInfo* evInfo);

	/* Clean up the SockInfo structure */
	void remsock(SockInfo* sinfo);
private:
	// 通知发送
	void notifySendRequest();
	
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

	// 处理curl_multi_info_read返回的响应
	void afterRequestDone(const CurlRequestPtr& request);
	
	// 内部定时器超时响应函数
	void innerTimerTimeOut(uint64_t req_uuid);

	
private:
	// 事件管理器
	// 唤醒FD
	int wakeupFd_;
	// 多路句柄管理器
	CURLM* curlm_;
	// 记录正在被监控的简单句柄的总个数
	int runningHandles_;
	// 记录上一次监控的简单句柄的总个数
	int prevRunningHandles_;
	// 停止标记
	bool stopped_;
	// 事件循环器
	EvLoopInfo evInfo_;
};

} // end namespace curl


#endif // __CURL_CURLMANAGER_H__
