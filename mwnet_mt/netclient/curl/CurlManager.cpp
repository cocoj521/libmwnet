#include "CurlManager.h"
#include "RequestManager.h"
#include <mwnet_mt/base/Atomic.h>
#include <mwnet_mt/base/Logging.h>
#include <assert.h>
#include <signal.h>
#include <sys/eventfd.h>
#include <sys/stat.h>
#include <sys/cdefs.h>

using namespace curl;
// ������Ϣ��ͳ����
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

CurlManager::CurlManager()
	: wakeupFd_(createEventfd()),
	  curlm_(CHECK_NOTNULL(curl_multi_init())),
	  runningHandles_(0),
	  prevRunningHandles_(0),
	  stopped_(false)
{
	curl_multi_setopt(curlm_, CURLMOPT_SOCKETFUNCTION, &CurlManager::curlmSocketOptCb);
	curl_multi_setopt(curlm_, CURLMOPT_SOCKETDATA, this);
	curl_multi_setopt(curlm_, CURLMOPT_TIMERFUNCTION, &CurlManager::curlmTimerCb);
	curl_multi_setopt(curlm_, CURLMOPT_TIMERDATA, this);

	// û��Ҫ���ã��ϲ�����Ҫ����ҵ���߼����Ƶģ��ײ�ȡĬ��ֵ������
	//if (maxtotalconns > 0) curl_multi_setopt(curlm_, CURLMOPT_MAX_TOTAL_CONNECTIONS, maxtotalconns);
	//if (maxhostconns > 0) curl_multi_setopt(curlm_, CURLMOPT_MAX_HOST_CONNECTIONS, maxhostconns);
	//if (maxconns > 0) curl_multi_setopt(curlm_, CURLMOPT_MAXCONNECTS, maxconns);
	//curl_multi_setopt(curlm_, CURLMOPT_MAX_HOST_CONNECTIONS, 100);
	curl_multi_setopt(curlm_, CURLMOPT_MAXCONNECTS, 100);
}

CurlManager::~CurlManager()
{
	curl_multi_cleanup(curlm_);
}

int CurlManager::runCurlEvLoop(const std::shared_ptr<CountDownLatch>& latch)
{
	// ��ʼ��evbase
	if(NULL != (evInfo_.evbase = event_base_new()))
	{
		LOG_INFO << "event_base_new:" << evInfo_.evbase;

		// ע�ỽ���¼��������¼�ѭ��
		int nRet = event_assign(&evInfo_.wakeup_event, evInfo_.evbase, wakeupFd_, EV_READ|EV_PERSIST, &CurlManager::onRecvWakeUpNotify, this);
		LOG_INFO << "event_assign wakeup event:" << nRet;
		
		// �����¼�ѭ��
		nRet = event_add(&evInfo_.wakeup_event, NULL);
		LOG_INFO << "event_add wakeup event:" << nRet;

		// ע��һ����ʱ�¼�,����curl�ĳ�ʱ����
		nRet = evtimer_assign(&evInfo_.timer_event, evInfo_.evbase, &CurlManager::onRecvEvTimerNotify, this);
		LOG_INFO << "evtimer_assign curl timecb:" << nRet;
		
		// ע��һ����ʱ�¼�,����request�ĳ�ʱδ�ص�����
		//nRet = evtimer_assign(&evInfo_.request_timeout_event, evInfo_.evbase, &CurlManager::onRecvEvTimerNotify, this);
		//LOG_INFO << "evtimer_assign request timeout event:" << nRet;

		latch->countDown();
		
		// �����¼�ѭ��(������....������event_base_loopbreak() or event_base_loopexit() ʱ�ŷ���)
		nRet = event_base_dispatch(evInfo_.evbase);
		LOG_INFO << "event_base_dispatch return:" << nRet << ",evloop exit";
				
		// ѭ���˳�ʱ����evloop
		event_del(&evInfo_.wakeup_event);
		close(wakeupFd_);
		event_del(&evInfo_.timer_event);
		//event_del(&evInfo_.request_timeout_event);
		event_base_free(evInfo_.evbase);
	}

	return 0;
}

int CurlManager::curlmSocketOptCb(CURL* c, int fd, int what, void* userp, void* socketp)
{
	if (userp)
	{
		CurlManager* cm = static_cast<CurlManager*>(userp);
		cm->handleCurlmSocketOptCb(c, fd, what, socketp);
	}
	
	return 0;
}

void CurlManager::handleCurlmSocketOptCb(CURL* c, int fd, int what, void* socketp)
{
	const char *whatstr[]={ "none", "IN", "OUT", "INOUT", "REMOVE" };

	LOG_DEBUG << "cm:" << this << ",curl:" << c << ",fd:" << fd << ",socketp:" << socketp << ",what:" << whatstr[what];
	
	SockInfo* sInfo = static_cast<SockInfo*>(socketp);

	if (what == CURL_POLL_REMOVE && sInfo)
	{
		remsock(sInfo);
	}
	else
	{
		if (!sInfo)
		{
			addsock(fd, c, what, &evInfo_);
		}
		else
		{
			setsock(sInfo, fd, c, what, &evInfo_);
		}
	}

}

void CurlManager::addsock(int fd, CURL* c, int action, EvLoopInfo* evInfo)
{
	SockInfo* sInfo = static_cast<SockInfo*>(malloc(sizeof(SockInfo)));
	if (sInfo)
	{
		LOG_DEBUG << "malloc sInfo";

		memset(sInfo, 0, sizeof(SockInfo));

		setsock(sInfo, fd, c, action, evInfo);

		curl_multi_assign(curlm_, fd, sInfo);
	}
	else
	{
		LOG_ERROR << "malloc sInfo fail";
	}
}

/* Assign information to a SockInfo structure */
void CurlManager::setsock(SockInfo* sInfo, int fd, CURL* c, int action, EvLoopInfo* evInfo)
{
	short evtp = static_cast<short>((action&CURL_POLL_IN?EV_READ:0)|(action&CURL_POLL_OUT?EV_WRITE:0)|EV_PERSIST);
	if (sInfo && c && evInfo)
	{
		sInfo->fd = fd;
		sInfo->action = action;
		sInfo->easy = c;

		if (event_initialized(&sInfo->ev)) 
		{
			event_del(&sInfo->ev);
		}
	
		event_assign(&sInfo->ev, evInfo->evbase, fd, evtp, onRecvEvOptNotify, this);
		event_add(&sInfo->ev, NULL);
	}	
}

/* Clean up the SockInfo structure */
void CurlManager::remsock(SockInfo* sInfo)
{
	if (sInfo)
	{
		if (event_initialized(&sInfo->ev))
		{
			event_del(&sInfo->ev);
		}

		free(sInfo);

		LOG_DEBUG << "free sInfo";
	}
}

int CurlManager::curlmTimerCb(CURLM* curlm, long ms, void* userp)
{
	LOG_DEBUG << "CurlManager::curlmTimerCb";
	
	if (userp)
	{
		CurlManager* cm = static_cast<CurlManager*>(userp);
		cm->handleCurlmTimerCb(ms);
	}
	return 0;
}

void CurlManager::handleCurlmTimerCb(long ms)
{
	struct timeval timeout;
	timeout.tv_sec = ms / 1000;
	timeout.tv_usec = (ms % 1000)*1000;
	
	if (ms == 0)
	{
		curl_multi_socket_action(curlm_, CURL_SOCKET_TIMEOUT, 0, &runningHandles_);
	}
	else if (ms == -1)
	{
		evtimer_del(&evInfo_.timer_event);
	}
	else
	{
		evtimer_add(&evInfo_.timer_event, &timeout);
	}
}

void CurlManager::onRecvEvTimerNotify(int fd, short evtp, void *arg)
{
	if (arg)
	{
		CurlManager* cm = static_cast<CurlManager*>(arg);
		cm->handleEvTimerCb();
	}
}

void CurlManager::handleEvTimerCb()
{
	curl_multi_socket_action(curlm_, CURL_SOCKET_TIMEOUT, 0, &runningHandles_);
	check_multi_info();
}

void CurlManager::onRecvEvOptNotify(int fd, short evtp, void *arg)
{
	if (arg)
	{
		CurlManager* cm = static_cast<CurlManager*>(arg);
		cm->handleEvOptCb(fd, evtp);
	}
}

void CurlManager::handleEvOptCb(int fd, short evtp)
{
	int action = (evtp & EV_READ ? CURL_POLL_IN : 0) | (evtp & EV_WRITE ? CURL_POLL_OUT : 0);

	curl_multi_socket_action(curlm_, fd, action, &runningHandles_);

	check_multi_info();
	
	if (runningHandles_ <= 0)
	{
		if (evtimer_pending(&evInfo_.timer_event, NULL))
		{
			evtimer_del(&evInfo_.timer_event);
		}
	}
}

CurlRequestPtr CurlManager::getRequest(const std::string& url, bool bKeepAlive, int req_type, int http_ver)
{
	uint64_t req_uuid = totalCount_.incrementAndGet();

	// �ȴӻ���վ���ã��޿��õ���new
	CurlRequestPtr p = HttpRecycleRequest::GetInstance().get();
	
	if (nullptr == p)
	{
		p.reset(new CurlRequest(url, req_uuid, bKeepAlive, req_type, http_ver));
	}
	else
	{
		p->initCurlRequest(url, req_uuid, bKeepAlive, req_type, http_ver);
	}

	LOG_DEBUG << "getRequest CurlRequestPtr = " << p.get() << " req_uuid = " << req_uuid;

	return p;
}

void CurlManager::notifySendRequest()
{
	uint64_t one = 1;
	write(wakeupFd_, &one, sizeof one);
}

void CurlManager::onRecvWakeUpNotify(int fd, short evtp, void *arg)
{
	if (arg)
	{
		CurlManager* cm = static_cast<CurlManager*>(arg);
		cm->handleWakeUpCb();
	}
}

void CurlManager::handleWakeUpCb()
{	
	uint64_t one = 1;
	read(wakeupFd_, &one, sizeof one);

	// ���Ѻ󣬴Ӵ���������ȡ���ݷ���
	checkNeedSendRequest();
}

// ��������
int CurlManager::sendRequest(const CurlRequestPtr& request)
{
	int nRet = 0;

	request->req_inque_time_ = Timestamp::GetCurrentTimeUs();
	
	// ���ж��Ƿ񳬹���󲢷���
	if (HttpRequesting::GetInstance().isFull())
	{
		nRet = 3;
	}
	else
	{
		// �����������
		if (!HttpWaitRequest::GetInstance().add(request))
		{
			nRet = 2;
		}
	}
	
	// ֪ͨ����
	notifySendRequest();

	return nRet;
}

void CurlManager::sendRequestInLoop(const CurlRequestPtr& request)
{
	/*
	// ��һ���ڲ���ʱ����ⳬʱ���Է�curl���ص�����ʱʱ����curl����������3��
	long tm = request->entire_timeout_ + 3;
	struct timeval timeout;
	timeout.tv_sec = tm / 1000;
	timeout.tv_usec = (tm % 1000) * 1000;

	evtimer_add(&evInfo_.request_timeout_event, &timeout);
	*/
	// �ȼ��뷢����
	HttpRequesting::GetInstance().add(request);
	// ��������
	request->request(curlm_);
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
		
		// ����Ƿ�����Ҫ���͵�����
		checkNeedSendRequest();
	}
	prevRunningHandles_ = runningHandles_;

	// û���ڵȴ�������¼�ʱ�ſ����˳�
	if (runningHandles_ <= 0 && stopped_)
	{
		event_base_loopbreak(evInfo_.evbase);
	}
}
void CurlManager::afterRequestDone(const CurlRequestPtr& request)
{
	// �Ƴ�request����ʱ��ⶨʱ��
	//evtimer_del(&evInfo_.request_timeout_event);
	// �������Ƴ�������
	removeMultiHandle(request);
	// ��������,����������ǻ������û���ֱ���ͷ�
	recycleRequest(request);
}

void CurlManager::forceCancelRequest(uint64_t req_uuid)
{
	/*
	LOG_DEBUG << "CurlManager::forceCancelRequest " << req_uuid;

	CurlRequestPtr request = HttpRequesting::GetInstance().find(req_uuid);
	if (request)
	{
		// �Ƴ��ڲ���ʱ��
		loop_->cancel(request->timerid_);
		// �������Ƴ�������
		removeMultiHandle(request);
		// ������ӷ����ж����Ƴ�
		HttpRequesting::GetInstance().remove(request);
		// �ص��ж�����
		loop_->queueInLoop(std::bind(&CurlRequest::done, request, 10055, "Request aborted manually"));
	}
	*/
}

// �����ж�����(�ڲ�����)
void CurlManager::forceCancelRequestInner(const CurlRequestPtr& request)
{
	/*
	LOG_DEBUG << "req_uuid = " << request->getReqUUID();
	
	// �Ƴ��ڲ���ʱ��
	loop_->cancel(request->timerid_);
	// �������Ƴ�������
	removeMultiHandle(request);
	// �ص��ж�����
	loop_->queueInLoop(std::bind(&CurlRequest::done, request, 10055, "Request aborted manually"));
	*/
}

// �ڲ���ʱ����ʱ��Ӧ����
void CurlManager::innerTimerTimeOut(uint64_t req_uuid)
{
	/*
	LOG_DEBUG << "CurlManager::innerTimerTimeOut";

	CurlRequestPtr request = HttpRequesting::GetInstance().find(req_uuid);
	if (request)
	{
		// �������Ƴ�������
		removeMultiHandle(request);
		// ������ӷ����ж����Ƴ�
		HttpRequesting::GetInstance().remove(request);
		// �ص��ڲ���ʱ����ʱ
		loop_->queueInLoop(std::bind(&CurlRequest::done, request, 10028, "Timer of httpclient was reached"));
		//request->done(10028, "Inner Timer TimeOut");
	}
	*/
}

// �������Ƴ�������
void CurlManager::removeMultiHandle(const CurlRequestPtr& request)
{
	request->removeMultiHandle(curlm_);
}

// ��������,����������ǻ������û���ֱ���ͷ�
void CurlManager::recycleRequest(const CurlRequestPtr& request)
{
	// ������ӷ����ж����Ƴ�
	HttpRequesting::GetInstance().remove(request);

	// ��������
	HttpRecycleRequest::GetInstance().recycle(request);
	// �����ӣ��������գ������ã���
	//.............
}

// ����Ƿ�����Ҫ���͵�����
void CurlManager::checkNeedSendRequest()
{
	CurlRequestPtr request = nullptr;
	// ����Ƿ�����Ҫ���͵�����....
	while (!HttpRequesting::GetInstance().isFull() 
		&& (request=HttpWaitRequest::GetInstance().get()) != nullptr)
	{
		sendRequestInLoop(request);
	}
}
