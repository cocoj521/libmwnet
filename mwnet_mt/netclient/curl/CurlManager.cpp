#include "CurlManager.h"
#include "RequestManager.h"
#include <mwnet_mt/base/Atomic.h>
#include <mwnet_mt/base/Logging.h>
#include <mwnet_mt/net/SocketsOps.h>
#include <assert.h>
#include <signal.h>
#include <sys/eventfd.h>

using namespace curl;
// 发送消息总统计数
mwnet_mt::AtomicUint64 totalCount_;

void CurlManager::initialize(Option opt)
{
	curl_global_init(opt == kCURLnossl ? CURL_GLOBAL_NOTHING : CURL_GLOBAL_SSL);
	//curl_global_init(CURL_GLOBAL_ALL);
}

void CurlManager::uninitialize()
{
	curl_global_cleanup();
}

void CurlManager::addEvLoop(void* p, int fd, int what)
{
	// 加入事件循环
	curl_evmgr_->addEvLoop(p, fd, what,
							std::bind(&CurlManager::onReadEventCallBack, this, fd),
							std::bind(&CurlManager::onWriteEventCallBack, this, fd));
	// 根据what的值,注册要响应的事件
	curl_evmgr_->optEvLoop(p, fd, what);
}

void CurlManager::optEvLoop(void* p, int fd, int what)
{
	// 根据what的值,注册要响应的事件
	curl_evmgr_->optEvLoop(p, fd, what);
}

void CurlManager::delEvLoop(void* p, int fd, int what)
{
	// 移除事件循环
	curl_evmgr_->delEvLoop(p, fd, what);
}

int CurlManager::curlmSocketOptCbInLoop(CURL* c, int fd, int what, void* socketp)
{
<<<<<<< HEAD
=======
	const char *whatstr[]={ "none", "IN", "OUT", "INOUT", "REMOVE" };
	LOG_DEBUG << "CurlManager::curlmSocketOptCbInLoop [" << this << "] [" << socketp << "] fd=" << fd
			  << " what=" << whatstr[what];

<<<<<<< HEAD
>>>>>>> parent of 755b2ca (浼樺寲curlHttpClient)
=======
>>>>>>> parent of 755b2ca (浼樺寲curlHttpClient)
	// 事件移除
	if (what == CURL_POLL_REMOVE) 
	{
		// 将数据设置为与内部套接字解除关联
		curl_multi_assign(curlm_, fd, NULL);
		// 放入loop,移除事件循环
		loop_->queueInLoop(std::bind(&CurlManager::delEvLoop, this, socketp, fd, what));
	}
	else 
	{
		if (!socketp)
		{
			// 生成EV
			void* p = curl_evmgr_->getNewEv(fd);
			// 将数据设置为与内部套接字添加关联
			CURLMcode rc = curl_multi_assign(curlm_, fd, p);
			if (0 == rc)
			{
				LOG_DEBUG << "curl_multi_assign:" << fd 
						<< "-" << p
						<< "-" << rc 
						<< "-" << curl_multi_strerror(rc);
				
				// 放入loop,加入事件循环
				loop_->queueInLoop(std::bind(&CurlManager::addEvLoop, this, p, fd, what));
			}
			else
			{
				LOG_DEBUG << "curl_multi_assign:" << fd 
						<< "-" << p
						<< "-" << rc 
						<< "-" << curl_multi_strerror(rc);
				
				curl_evmgr_->delEv(p);
			}
		}
		else
		{
			// 放入loop,根据what的值,注册要响应的事件
			loop_->queueInLoop(std::bind(&CurlManager::optEvLoop, this, socketp, fd, what));
		}
	}
	
	return 0;
}

int CurlManager::curlmSocketOptCb(CURL* c, int fd, int what, void* userp, void* socketp)
{
	CurlManager* cm = static_cast<CurlManager*>(userp);
	
	if (NULL == cm || fd <= 0) return 0;
	
	const char *whatstr[] = { "none", "IN", "OUT", "INOUT", "REMOVE" };
	LOG_DEBUG << "cm = " << cm << " curl = " << c << " fd = " << fd
		<< " userp = " << userp << " socketp = " << socketp  << " what = " << whatstr[what];

	cm->curlmSocketOptCbInLoop(c, fd, what, socketp);
		
	return 0;
}

int CurlManager::curlmTimerCb(CURLM* curlm, long ms, void* userp)
{	
	LOG_DEBUG << "cm = " << curlm << " userp = " << userp << " ms = " << ms;
	
	CurlManager* cm = static_cast<CurlManager*>(userp);
	if (NULL != cm)
	{		
		// ms>0 ms=-1都不用处理
		if (ms == 0)
		{
			// loop中执行			
			cm->loop_->runInLoop(std::bind(&CurlManager::curlmTimerCbInLoop, cm, 0L));
		}
		else if (ms > 0)
		{
			// 创建timer
			cm->loop_->queueInLoop(std::bind(&CurlManager::createTimer, cm, ms));
		}
		else
		{	
			// 取消timer
			cm->loop_->queueInLoop(std::bind(&CurlManager::cancelTimer, cm));
		}
	}
	
	return 0;
}

void CurlManager::cancelTimer()
{
	loop_->cancel(timerid_);
}

void CurlManager::createTimer(long ms)
{
	// 先取消timer
	loop_->cancel(timerid_);
	
	// 再创建
	timerid_ = loop_->runAfter(static_cast<int>(ms)/1000.0, 
				std::bind(&CurlManager::curlmTimerCbInLoop, shared_from_this(), ms));
}

void CurlManager::curlmTimerCbInLoop(long ms)
{
	if (0 == ms)
	{
		// 先取消已有timer
		loop_->cancel(timerid_);
	
		curl_multi_socket_action(curlm_, CURL_SOCKET_TIMEOUT, 0, &runningHandles_);
	}
	else
	{
		curl_multi_socket_action(curlm_, CURL_SOCKET_TIMEOUT, 0, &runningHandles_);
		check_multi_info();
	}
	
}

int createEventfd()
{
	int evtfd = ::eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
	if (evtfd < 0)
	{
		LOG_SYSERR << "Failed in eventfd";
		abort();
	}
	return evtfd;
}

CurlManager::CurlManager(EventLoop* loop)
	: loop_(loop),
	  curl_evmgr_(new CurlEvMgr(loop)),
	  wakeupFd_(createEventfd()),
      wakeupChannel_(new Channel(loop, wakeupFd_)),
	  curlm_(CHECK_NOTNULL(curl_multi_init())),
	  runningHandles_(0),
	  prevRunningHandles_(0)
{
	curl_multi_setopt(curlm_, CURLMOPT_SOCKETFUNCTION, &CurlManager::curlmSocketOptCb);
	curl_multi_setopt(curlm_, CURLMOPT_SOCKETDATA, this);
	curl_multi_setopt(curlm_, CURLMOPT_TIMERFUNCTION, &CurlManager::curlmTimerCb);
	curl_multi_setopt(curlm_, CURLMOPT_TIMERDATA, this);

	// 没必要设置，上层总是要根据业务逻辑控制的，底层取默认值：不限
	//if (maxtotalconns > 0) curl_multi_setopt(curlm_, CURLMOPT_MAX_TOTAL_CONNECTIONS, maxtotalconns);
	//if (maxhostconns > 0) curl_multi_setopt(curlm_, CURLMOPT_MAX_HOST_CONNECTIONS, maxhostconns);
	//if (maxconns > 0) curl_multi_setopt(curlm_, CURLMOPT_MAXCONNECTS, maxconns);

	// 设置唤醒回调
	wakeupChannel_->setReadCallback(std::bind(&CurlManager::onRecvWakeUpNotify, this));
	loop_->runInLoop(std::bind(&Channel::enableReading, get_pointer(wakeupChannel_)));
}

CurlManager::~CurlManager()
{
	// 析构要注意定时器，loop等安全退出
	//cancelTimer();
	curl_multi_cleanup(curlm_);
}

CurlRequestPtr CurlManager::getRequest(const std::string& url, bool bKeepAlive, int req_type, int http_ver)
{
	uint64_t req_uuid = totalCount_.incrementAndGet();

	// 先从回收站中拿，无可用的再new
	CurlRequestPtr p = HttpRecycleRequest::GetInstance().get();
	
	if (nullptr == p)
	{
		p.reset(new CurlRequest(url, req_uuid, bKeepAlive, req_type, http_ver));
	}
	else
	{
		p->initCurlRequest(url, req_uuid, bKeepAlive, req_type, http_ver);
	}

	LOG_DEBUG << " CurlRequestPtr = " << p.get() << " req_uuid = " << req_uuid;
		
	return p;
}
// 判断事件循环中是否还有未完成事件
size_t CurlManager::isLoopRunning()
{
	return loop_->queueSize();
}

void CurlManager::notifySendRequest()
{
	uint64_t one = 1;
	sockets::write(wakeupFd_, &one, sizeof one);
}

void CurlManager::onRecvWakeUpNotify()
{
	uint64_t one = 1;
	sockets::read(wakeupFd_, &one, sizeof one);
	
	// 唤醒后，从待发队列中取数据发送
	checkNeedSendRequest();
}

// 发送请求
int CurlManager::sendRequest(const CurlRequestPtr& request)
{
	int nRet = 0;

	request->req_inque_time_ = Timestamp::GetCurrentTimeUs();
	
	// 先判断是否超过最大并发数
	if (HttpRequesting::GetInstance().isFull())
	{
		nRet = 3;
	}
	else
	{
		// 加入待发队列
		if (!HttpWaitRequest::GetInstance().add(request))
		{
			nRet = 2;
		}
	}
	
	// 通知发送
	notifySendRequest();

	return nRet;
}

void CurlManager::sendRequestInLoop(const CurlRequestPtr& request)
{
	// 启一个内部定时器检测超时，以防curl不回调，超时时间在curl基础上增加3秒
	long tm = request->entire_timeout_+3;
	request->timerid_ = loop_->runAfter(static_cast<int>(tm)/1.0, 
						std::bind(&CurlManager::innerTimerTimeOut, shared_from_this(), request->getReqUUID()));
	// 先加入发送中
	HttpRequesting::GetInstance().add(request);
	// 发送请求
	request->request(this, curlm_, loop_);
}

void CurlManager::onReadEventCallBack(int fd)
{
	LOG_DEBUG << "onReadEventCallBack";

	curl_multi_socket_action(curlm_, fd, CURL_POLL_IN, &runningHandles_);
	check_multi_info();
}

void CurlManager::onWriteEventCallBack(int fd)
{
	LOG_DEBUG << "onWriteEventCallBack";
	curl_multi_socket_action(curlm_, fd, CURL_POLL_OUT, &runningHandles_);
	check_multi_info();
}

void CurlManager::check_multi_info()
{
	LOG_DEBUG << "prevRunningHandles_ = " << prevRunningHandles_ << " runningHandles_ = " << runningHandles_;
	if (prevRunningHandles_ > runningHandles_ || runningHandles_ == 0)
	{
		CURLMsg* msg = NULL;
		int left = 0;
		while ((msg = curl_multi_info_read(curlm_, &left)) != NULL)
		{
			if (msg->msg == CURLMSG_DONE)
			{
				CURL* c = msg->easy_handle;
				CURLcode retCode = msg->data.result;
				
				CurlRequestPtr request = HttpRequesting::GetInstance().find(c);
				if (request && request->getCurl() == c)
				{
					LOG_DEBUG <<"CurlRequestPtr = " << request.get() << " done = " << curl_easy_strerror(retCode);
					request->done(retCode, curl_easy_strerror(retCode));

					afterRequestDone(request);
				}
			}
		}
		
		// 检查是否还有需要发送的请求
		checkNeedSendRequest();
	}
	prevRunningHandles_ = runningHandles_;
}

void CurlManager::afterRequestDone(const CurlRequestPtr& request)
{
	// 移除内部定时器
	loop_->cancel(request->timerid_);
	// 将请求移出管理器
	removeMultiHandle(request);
	// 回收请求,视情况而定是回收重用还是直接释放
	recycleRequest(request);
}

void CurlManager::forceCancelRequest(uint64_t req_uuid)
{
	LOG_DEBUG << "CurlManager::forceCancelRequest " << req_uuid;

	CurlRequestPtr request = HttpRequesting::GetInstance().find(req_uuid);
	if (request)
	{
		// 移除内部定时器
		loop_->cancel(request->timerid_);
		// 将请求移出管理器
		removeMultiHandle(request);
		// 将请求从发送中队列移除
		HttpRequesting::GetInstance().remove(request);
		// 回调中断请求
		loop_->queueInLoop(std::bind(&CurlRequest::done, request, 10055, "Request aborted manually"));
	}
}

// 处理中断请求(内部调用)
void CurlManager::forceCancelRequestInner(const CurlRequestPtr& request)
{
	LOG_DEBUG << "req_uuid = " << request->getReqUUID();
	
	// 移除内部定时器
	loop_->cancel(request->timerid_);
	// 将请求移出管理器
	removeMultiHandle(request);
	// 回调中断请求
	loop_->queueInLoop(std::bind(&CurlRequest::done, request, 10055, "Request aborted manually"));
}

// 内部定时器超时响应函数
void CurlManager::innerTimerTimeOut(uint64_t req_uuid)
{
	LOG_DEBUG << "CurlManager::innerTimerTimeOut";

	CurlRequestPtr request = HttpRequesting::GetInstance().find(req_uuid);
	if (request)
	{
		// 将请求移出管理器
		removeMultiHandle(request);
		// 将请求从发送中队列移除
		HttpRequesting::GetInstance().remove(request);
		// 回调内部定时器超时
		loop_->queueInLoop(std::bind(&CurlRequest::done, request, 10028, "Timer of httpclient was reached"));
		//request->done(10028, "Inner Timer TimeOut");
	}
}

// 将请求移出管理器
void CurlManager::removeMultiHandle(const CurlRequestPtr& request)
{
	request->removeMultiHandle(curlm_);
}

// 回收请求,视情况而定是回收重用还是直接释放
void CurlManager::recycleRequest(const CurlRequestPtr& request)
{
	// 将请求从发送中队列移除
	HttpRequesting::GetInstance().remove(request);

	// 回收连接
	HttpRecycleRequest::GetInstance().recycle(request);
	// 长连接？？？回收？？重用？？
	//.............
}

// 检查是否还有需要发送的请求
void CurlManager::checkNeedSendRequest()
{
	CurlRequestPtr request = nullptr;
	// 检查是否有需要发送的请求....
	while (!HttpRequesting::GetInstance().isFull() 
		&& (request=HttpWaitRequest::GetInstance().get()) != nullptr)
	{
		sendRequestInLoop(request);
	}
}
