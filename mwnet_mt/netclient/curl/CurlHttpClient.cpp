#include "CurlHttpClient.h"
#include "CurlRequest.h"
#include "CurlManager.h"
#include "RequestManager.h"
#include <mwnet_mt/base/LogFile.h>
#include <mwnet_mt/base/Logging.h>
#include <mwnet_mt/base/CountDownLatch.h>
#include <mwnet_mt/net/EventLoop.h>
#include <mwnet_mt/net/EventLoopThread.h>
#include <mwnet_mt/net/EventLoopThreadPool.h>

#ifdef LIB_VERSION
const char *pVer = LIB_VERSION;
#endif

using namespace mwnet_mt;
using namespace mwnet_mt::net;
using namespace curl;

namespace MWNET_MT
{
namespace CURL_HTTPCLIENT
{

/////////////////////////////////////////////////////////////////////////////////////////
//   HttpRequest
/////////////////////////////////////////////////////////////////////////////////////////
HttpRequest::HttpRequest(const std::string& strUrl, HTTP_VERSION http_ver, HTTP_REQUEST_TYPE req_type, bool bKeepAlive)
	: keep_alive_(false),	
	  req_type_(req_type),
	  http_ver_(http_ver)
{
}

HttpRequest::~HttpRequest()
{
}

void HttpRequest::SetBody(std::string& strBody)
{
	// POST请求才设body,其他忽略
	if (HTTP_REQUEST_POST == req_type_)
	{
		CurlRequestPtr curl_req = boost::any_cast<CurlRequestPtr>(any_);
		if (curl_req)
		{
			curl_req->setBody(strBody);
		}
	}
}

void HttpRequest::SetHeader(const std::string& strField, const std::string& strValue)
{
	CurlRequestPtr curl_req = boost::any_cast<CurlRequestPtr>(any_);
	if (curl_req)
	{
		curl_req->setHeaders(strField, strValue);
	}
}
	
void HttpRequest::SetUserAgent(const std::string& strUserAgent)
{
	CurlRequestPtr curl_req = boost::any_cast<CurlRequestPtr>(any_);
	if (curl_req)
	{
		curl_req->setUserAgent(strUserAgent);
	}
}

void HttpRequest::SetContentType(const std::string& strContentType)
{
	CurlRequestPtr curl_req = boost::any_cast<CurlRequestPtr>(any_);
	if (curl_req)
	{
		curl_req->setHeaders("Content-Type", strContentType);
	}
}

void HttpRequest::SetTimeOut(int conn_timeout, int entire_timeout)
{
	CurlRequestPtr curl_req = boost::any_cast<CurlRequestPtr>(any_);
	if (curl_req)
	{
		curl_req->setTimeOut(conn_timeout, entire_timeout);
	}
}	

void HttpRequest::SetKeepAliveTime(int keep_idle, int keep_intvl)
{
	CurlRequestPtr curl_req = boost::any_cast<CurlRequestPtr>(any_);
	if (curl_req)
	{
		curl_req->setKeepAliveTime(keep_idle, keep_intvl);
	}
}
void HttpRequest::SetDnsCacheTimeOut(int cache_timeout)
{
	CurlRequestPtr curl_req = boost::any_cast<CurlRequestPtr>(any_);
	if (curl_req)
	{
		curl_req->setDnsCacheTimeOut(cache_timeout);
	}
}

void HttpRequest::SetContext(const boost::any& context)
{
	any_ = context;
}

const boost::any& HttpRequest::GetContext() const
{
	return any_;
}

//获取本次请求的惟一req_uuid
uint64_t HttpRequest::GetReqUUID() const
{
	uint64_t req_uuid = 0;
	CurlRequestPtr curl_req = boost::any_cast<CurlRequestPtr>(any_);
	if (curl_req)
	{
		req_uuid = curl_req->getReqUUID();
	}
	return req_uuid;
}



/////////////////////////////////////////////////////////////////////////////////////////
//   HttpResponse
/////////////////////////////////////////////////////////////////////////////////////////
HttpResponse::HttpResponse(int nRspCode,int nErrCode,
							const std::string& strErrDesc, 
							const std::string& strContentType, 
							std::string& strHeader, 
							std::string& strBody, 
							const std::string& strEffectiveUrl,
							const std::string& strRedirectUrl,
							uint32_t total, uint32_t connect, uint32_t namelookup_time,
							uint64_t rsp_time,uint64_t req_time,uint64_t req_inque_time,uint64_t req_uuid)
							:
							m_nRspCode(nRspCode),
							m_nErrCode(nErrCode),
							m_strErrDesc(strErrDesc),
							m_strContentType(strContentType),
							m_strEffectiveUrl(strEffectiveUrl),
							m_strRedirectUrl(strRedirectUrl),
							total_time_(total),
							connect_time_(connect),
							namelookup_time_(namelookup_time),
							rsp_time_(rsp_time),
							req_time_(req_time),
							req_inque_time_(req_inque_time),
							req_uuid_(req_uuid)
{
	m_strHeader.swap(strHeader);
	m_strBody.swap(strBody);
}

HttpResponse::~HttpResponse()
{

}

int HttpResponse::GetRspCode() const
{
	return m_nRspCode;
}

int HttpResponse::GetErrCode() const
{
	return m_nErrCode;
}

const std::string& HttpResponse::GetErrDesc() const
{
	return m_strErrDesc;
}

const std::string& HttpResponse::GetContentType() const
{
	return m_strContentType;
}

const std::string& HttpResponse::GetHeader() const
{
	return m_strHeader;
}

const std::string& HttpResponse::GetBody() const
{
	return m_strBody;
}

/*
curl_easy_perform()
    |
    |--NAMELOOKUP
    |--|--CONNECT
    |--|--|--APPCONNECT
    |--|--|--|--PRETRANSFER
    |--|--|--|--|--STARTTRANSFER
    |--|--|--|--|--|--TOTAL
    |--|--|--|--|--|--REDIRECT
*/
void HttpResponse::GetTimeConsuming(uint32_t& total, uint32_t& connect, uint32_t& namelookup) const
{
	total		= total_time_ / 1000;
	connect		= connect_time_ / 1000;
	namelookup	= namelookup_time_ / 1000;
}

//获取回应对应的请求整体耗时
uint32_t HttpResponse::GetReqTotalConsuming() const
{
	return total_time_ / 1000;
}

//获取回应对应的请求连接耗时
uint32_t HttpResponse::GetReqConnectConsuming() const
{
	return connect_time_ / 1000;
}

//获取回应对应的请求域名解析耗时
uint32_t HttpResponse::GetReqNameloopupConsuming() const
{
	return namelookup_time_ / 1000;
}

//获取回应返回的时间
uint64_t HttpResponse::GetRspRecvTime() const
{
	return rsp_time_;
}

//获取该回应对应的请求的请求时间
uint64_t HttpResponse::GetReqSendTime() const
{
	return req_time_;
}

//获取该回应对应的请求入队时间
uint64_t HttpResponse::GetReqInQueTime() const
{
	return req_inque_time_;
}

//获取该回应对应的请求UUID
uint64_t HttpResponse::GetReqUUID() const
{
	return req_uuid_;
}

//获取回应对应的请求的有效URL地址
const std::string& HttpResponse::GetEffectiveUrl() const
{
	return m_strEffectiveUrl;
}

//获取回应对应的请求的重定向URL地址
const std::string& HttpResponse::GetRedirectUrl() const
{
	return m_strRedirectUrl;
}

typedef std::shared_ptr<EventLoopThread> EventLoopThreadPtr;

////////////////////////////////////////////////////////////////////////////////////////////////////////
//   CurlHttpClient
////////////////////////////////////////////////////////////////////////////////////////////////////////
CurlHttpClient::CurlHttpClient() :
	m_pInvoker(NULL),
	m_pfunc_onmsg_cb(NULL),
	main_loop_(nullptr),
	io_loop_(nullptr),
	MaxTotalConns_(10000),
	MaxHostConns_(20),	
	IoThrNum_(4),
	MaxReqQueSize_(50000),
	exit_(false)
{
	total_req_ = 0;
	// 申请一个线程将loop跑在线程里
	EventLoopThreadPtr pLoopThr(new EventLoopThread(NULL, "mHttpCliMain"));
	EventLoop* pLoop = pLoopThr->startLoop();
	if (pLoop)
	{
		// 保存mainloop
		main_loop_ = pLoopThr;
	}
	// 初始化curl库
	CurlManager::initialize(CurlManager::kCURLssl);
}

CurlHttpClient::~CurlHttpClient()
{
	CurlManager::uninitialize();
}

void CurlHttpClient::DoneCallBack(const boost::any& _any, int errCode, const char* errDesc)
{
	CurlRequestPtr clientPtr = boost::any_cast<CurlRequestPtr>(_any);
	HttpResponsePtr http_response(new HttpResponse
									(clientPtr->getResponseCode(), 
									errCode, 
									std::string(errDesc), 
									clientPtr->getResponseContentType(), 
									clientPtr->getResponseHeader(), 
									clientPtr->getResponseBody(),
									clientPtr->getEffectiveUrl(),
									clientPtr->getRedirectUrl(),
									clientPtr->total_time_,
									clientPtr->connect_time_,
									clientPtr->namelookup_time_,
									clientPtr->rsp_time_,
									clientPtr->req_time_,
									clientPtr->req_inque_time_,
									clientPtr->getReqUUID()
									)
								 );	

	if (m_pfunc_onmsg_cb)
	{
		m_pfunc_onmsg_cb(m_pInvoker, clientPtr->getReqUUID(), clientPtr->getContext(), http_response);
	}
	
	--wait_rsp_;
	std::shared_ptr<CountDownLatch> latchPtr(boost::any_cast<std::shared_ptr<CountDownLatch>>(latch_));
	latchPtr->countDown();
	
	time_t tNow = time(NULL);
	static time_t tLast = time(NULL);
	static uint64_t total_req = 0;
	time_t nTmDiff = tNow - tLast;
	if (nTmDiff >= 1)
	{
		uint64_t now_req = total_req_.load();	
		uint64_t nSpeed = now_req - total_req;
		total_req = now_req;
		tLast = tNow;
		LOG_DEBUG << "SPEED:" << nSpeed/nTmDiff << " TOTAL:" << now_req << " WAIT:" << wait_rsp_.load();
	}
}

bool CurlHttpClient::InitHttpClient(void* pInvoker,
									pfunc_onmsg_cb pFnOnMsg,
									int nIoThrNum,
									int nMaxReqQueSize,
									int nMaxTotalConns,
									int nMaxHostConns)
{
	bool bRet = false;	

	m_pInvoker = pInvoker;
	m_pfunc_onmsg_cb = pFnOnMsg;
	MaxTotalConns_ = nMaxTotalConns;
	MaxHostConns_ = nMaxHostConns;
	IoThrNum_ = nIoThrNum;
	nMaxReqQueSize>nMaxTotalConns?nMaxReqQueSize=nMaxTotalConns:1;
	MaxReqQueSize_   = nMaxReqQueSize;
	total_req_ = 0;
	wait_rsp_ = 0;

	HttpRequesting::GetInstance().setMaxSize(MaxTotalConns_, MaxHostConns_);
	HttpWaitRequest::GetInstance().setMaxSize(MaxReqQueSize_);
	HttpRecycleRequest::GetInstance().setMaxSize(MaxTotalConns_);

	EventLoopThreadPtr main_loop(boost::any_cast<EventLoopThreadPtr>(main_loop_));
	if (main_loop)
	{
		EventLoop* pMainLoop = main_loop->getloop();
		// 创建并启动IO线程
		std::shared_ptr<EventLoopThreadPool> io_loop(new EventLoopThreadPool(pMainLoop, "mHttpCliIo"));
		io_loop->setThreadNum(nIoThrNum);
		// 保存io_loop
		io_loop_ = io_loop;
		
		std::shared_ptr<CountDownLatch> latchPtr(new CountDownLatch(nIoThrNum));
		latch_ = latchPtr;
		
		pMainLoop->runInLoop(std::bind(&CurlHttpClient::Start, this));

		latchPtr->wait();

		exit_ = false;
		
		bRet = true;
	}
	return bRet;
}					

void CurlHttpClient::Start()
{
	std::shared_ptr<EventLoopThreadPool> io_loop(boost::any_cast<std::shared_ptr<EventLoopThreadPool>>(io_loop_));
	if (io_loop)
	{
		io_loop->start();
		std::shared_ptr<CountDownLatch> latchPtr(boost::any_cast<std::shared_ptr<CountDownLatch>>(latch_));
		httpclis_.clear();
		// 初始化若干个curl对象
		for (int i = 0; i < IoThrNum_; i++)
		{
			std::shared_ptr<CurlManager> curlm(new CurlManager(io_loop->getNextLoop()));
			httpclis_.push_back(curlm);
			latchPtr->countDown();
		}
	}
}

//获取HttpRequest
HttpRequestPtr CurlHttpClient::GetHttpRequest(const std::string& strUrl, HTTP_VERSION http_ver,	 HTTP_REQUEST_TYPE req_type, bool bKeepAlive)
{
	// 生成curlrequest
	CurlRequestPtr curl_request = CurlManager::getRequest(strUrl, bKeepAlive, req_type, http_ver);
	
	// 设置完成回调
	curl_request->setDoneCallback(std::bind(&CurlHttpClient::DoneCallBack,this, _1, _2, _3));
	//std::placeholders::_1
	
	// 生成httprequest
	HttpRequestPtr http_request(new HttpRequest(strUrl, http_ver, req_type, bKeepAlive));
	
	// 保存curl_request
	http_request->SetContext(curl_request);

	return http_request;
}

//发送http请求
int  CurlHttpClient::SendHttpRequest(const HttpRequestPtr& request, const boost::any& params, bool WaitORreturnIfOverMaxTotalConns)
{
	int nRet = 1;

	int n = (++wait_rsp_ - MaxTotalConns_);

	if (!exit_)
	{
		std::shared_ptr<CountDownLatch> latchPtr(boost::any_cast<std::shared_ptr<CountDownLatch>>(latch_));

		if (n > 0 && WaitORreturnIfOverMaxTotalConns) 
		{
			latchPtr->addAndWait(n);
		}
		
		// 取发送中队列未满的管理器
		uint64_t count = total_req_.load();
		uint64_t index = count % static_cast<uint64_t>(httpclis_.size());
		boost::any& _any = httpclis_[index];
		
		if (!_any.empty())
		{
			std::shared_ptr<CurlManager> curlm = boost::any_cast<std::shared_ptr<CurlManager>>(_any);
		
			CurlRequestPtr curl_req = boost::any_cast<CurlRequestPtr>(request->GetContext());
			if (curl_req)
			{		
				curl_req->setContext(params);
				nRet = curlm->sendRequest(curl_req);
			}
		}

		if (0 == nRet) 
		{
			++total_req_;
		}
		else
		{
			--wait_rsp_;
		}
	}
	else
	{
		--wait_rsp_;
	}
	return nRet;
}

void CurlHttpClient::CancelHttpRequest(const HttpRequestPtr& request)
{
	CurlRequestPtr curl_req = boost::any_cast<CurlRequestPtr>(request->GetContext());
	if (curl_req)
	{
		curl_req->forceCancel();
	}
}

// 获取待回应数量
int  CurlHttpClient::GetWaitRspCnt() const
{
	return wait_rsp_.load();
}

// 获取待发请求数量
int  CurlHttpClient::GetWaitReqCnt() const
{
	return HttpWaitRequest::GetInstance().size();
}

void CurlHttpClient::ExitHttpClient()
{
	if (!exit_)
	{
		// 置退出标记
		exit_ = true;
		// 最多等多少次
		int maxWaitCnt = 30;
		while (GetWaitRspCnt() > 0)
		{
			// 将所有未回调的请求全部回调
			HttpWaitRequest::GetInstance().clear();
			HttpRequesting::GetInstance().clear();
			HttpRecycleRequest::GetInstance().clear();

			LOG_INFO << "CurlHttpClient::ExitHttpClient WaitRspCnt:" << GetWaitRspCnt();

			if (--maxWaitCnt <= 0)
			{
				break;
			}
			usleep(1000 * 100);
		}
	}
	LOG_INFO << "CurlHttpClient::ExitHttpClient";
}

}
}
