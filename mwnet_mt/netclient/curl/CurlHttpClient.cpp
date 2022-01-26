#include "CurlHttpClient.h"
#include "CurlRequest.h"
#include "CurlManager.h"
#include "RequestManager.h"
#include <mwnet_mt/base/LogFile.h>
#include <mwnet_mt/base/Logging.h>
#include <mwnet_mt/base/CountDownLatch.h>
#include <mwnet_mt/base/Condition.h>
#include <mwnet_mt/base/Thread.h>
#include <mwnet_mt/base/ThreadPool.h>

#ifdef LIB_VERSION
const char *pVer = LIB_VERSION;
#endif

using namespace mwnet_mt;
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
	LOG_DEBUG << "new";
}

HttpRequest::~HttpRequest()
{
	LOG_DEBUG << "del";
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
typedef std::shared_ptr<Thread> ThreadPtr;
typedef std::shared_ptr<ThreadPool> ThreadPoolPtr;

////////////////////////////////////////////////////////////////////////////////////////////////////////
//   CurlHttpClient
////////////////////////////////////////////////////////////////////////////////////////////////////////
CurlHttpClient::CurlHttpClient() :
	m_pInvoker(NULL),
	m_pfunc_onmsg_cb(NULL),
	io_loop_(nullptr),
	MaxTotalConns_(10000),
	MaxHostConns_(20),	
	IoThrNum_(4),
	MaxReqQueSize_(50000),
	exit_(false)
{
	total_req_ = 0;
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
	std::shared_ptr<Condition> conditionPtr(boost::any_cast<std::shared_ptr<Condition>>(condition_));
	conditionPtr->notifyAll();
	
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

// IO线程池初始化成功回调函数
static void func_IoThrInit(const std::shared_ptr<CountDownLatch>& latch)
{
	latch->countDown();
}

bool CurlHttpClient::InitHttpClient(void* pInvoker,
									pfunc_onmsg_cb pFnOnMsg,
									int nIoThrNum,
									int nMaxReqQueSize,
									int nMaxTotalConns,
									int nMaxHostConns)
{
	bool bRet = false;	

	// 创建IO线程池
	ThreadPoolPtr pIoThr(new ThreadPool("mHttpCliIo"));
	if (pIoThr)
	{
		m_pInvoker = pInvoker;
		m_pfunc_onmsg_cb = pFnOnMsg;
		MaxTotalConns_ = nMaxTotalConns;
		MaxHostConns_ = nMaxHostConns;
		IoThrNum_ = nIoThrNum;
		nMaxReqQueSize > nMaxTotalConns ? nMaxReqQueSize = nMaxTotalConns : 1;
		MaxReqQueSize_ = nMaxReqQueSize;
		total_req_ = 0;
		wait_rsp_ = 0;

		HttpRequesting::GetInstance().setMaxSize(MaxTotalConns_, MaxHostConns_);
		HttpWaitRequest::GetInstance().setMaxSize(MaxReqQueSize_);
		HttpRecycleRequest::GetInstance().setMaxSize(MaxTotalConns_);

		// 保存io_loop
		io_loop_ = pIoThr;

		// 启动线程池
		std::shared_ptr<CountDownLatch> latch(new CountDownLatch(nIoThrNum));
		latch_ = latch;
		pIoThr->setThreadInitCallback(std::bind(func_IoThrInit, latch));		
		pIoThr->start(nIoThrNum);
		latch->wait();
		
		std::shared_ptr<MutexLock> mutexPtr(new MutexLock());
		mutexLock_ = mutexPtr;
		std::shared_ptr<Condition> conditionPtr(new Condition(*mutexPtr));
		condition_ = conditionPtr;
				
		// 初始化curl库
		CurlManager::initialize(CurlManager::kCURLssl);
		
		// 将curl管理器跑在线程池里
		for (int i = 0; i < IoThrNum_; i++)
		{
			latch->resetCount(1);
			pIoThr->run(std::bind(&CurlHttpClient::Start, this));
			latch->wait();
		}
		
		bRet = true;
	}
	return bRet;
}					

void CurlHttpClient::Start()
{
	std::shared_ptr<CountDownLatch> latch(boost::any_cast<std::shared_ptr<CountDownLatch>>(latch_));
	std::shared_ptr<CurlManager> curlm(new CurlManager());
	boost::any curlm_any = curlm;
	if (!curlm_any.empty())
	{
		httpclis_.push_back(curlm_any);
		curlm->runCurlEvLoop(latch);
	}
	else
	{
		latch->countDown();
		LOG_ERROR << "start one curlm failed";
	}
}

//获取HttpRequest
HttpRequestPtr CurlHttpClient::GetHttpRequest(const std::string& strUrl, HTTP_VERSION http_ver,	 HTTP_REQUEST_TYPE req_type, bool bKeepAlive)
{
	// 生成curlrequest
	CurlRequestPtr curl_request = CurlManager::getRequest(strUrl, bKeepAlive, req_type, http_ver);
	
	// 设置完成回调
	curl_request->setDoneCallback(std::bind(&CurlHttpClient::DoneCallBack, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
	//std::placeholders::_1
	
	// 生成httprequest
	HttpRequestPtr http_request(new HttpRequest(strUrl, http_ver, req_type, bKeepAlive));

	// 保存curl_request
	http_request->SetContext(curl_request);

	return http_request;
}

//发送http请求
int  CurlHttpClient::SendHttpRequest(const HttpRequestPtr& request, const boost::any& params, bool WaitOrReturnIfOverMaxTotalConns)
{
	int nRet = 1;

	if (!exit_)
	{
		std::shared_ptr<Condition> conditionPtr(boost::any_cast<std::shared_ptr<Condition>>(condition_));
		//无可用连接则等待
		while (wait_rsp_.load() - MaxTotalConns_ >= 0)
		{
			if (WaitOrReturnIfOverMaxTotalConns) conditionPtr->wait();
		}
		++wait_rsp_;
		
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

	return nRet;
}

void CurlHttpClient::CancelHttpRequest(const HttpRequestPtr& request)
{
	/*
	CurlRequestPtr curl_req = boost::any_cast<CurlRequestPtr>(request->GetContext());
	if (curl_req)
	{
		curl_req->forceCancel();
	}
	*/
}

// 获取待回应数量
size_t  CurlHttpClient::GetWaitRspCnt() const
{
	return wait_rsp_.load();
}

// 获取待发请求数量
size_t  CurlHttpClient::GetWaitReqCnt() const
{
	/*
	boost::any _any = httpclis_[0];
	size_t tCnt = 0;
	if (!_any.empty())
	{
		std::shared_ptr<CurlManager> curlm = boost::any_cast<std::shared_ptr<CurlManager>>(_any);
		tCnt = curlm->loopSize();
	}
	return HttpWaitRequest::GetInstance().size() + tCnt;
	*/
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
