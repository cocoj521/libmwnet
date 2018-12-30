#include "CurlHttpClient.h"
#include "CurlRequest.h"
#include "CurlManager.h"
#include "RequestManager.h"
#include <mwnet_mt/base/LogFile.h>
#include <mwnet_mt/base/Logging.h>
#include <mwnet_mt/base/CountDownLatch.h>
#include <mwnet_mt/base/Thread.h>
#include <mwnet_mt/base/ThreadPool.h>


using namespace mwnet_mt;
using namespace curl;

namespace MWNET_MT
{
namespace CURL_HTTPCLIENT
{

/////////////////////////////////////////////////////////////////////////////////////////
//   HttpRequest
/////////////////////////////////////////////////////////////////////////////////////////
HttpRequest::HttpRequest(const std::string& strUrl, bool bKeepAlive, HTTP_REQUEST_TYPE req_type)
	: keep_alive_(bKeepAlive),	
	  req_type_(req_type)
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
							double total, double connect, double namelookup_time)
							:
							m_nRspCode(nRspCode),
							m_nErrCode(nErrCode),
							m_strErrDesc(strErrDesc),
							m_strContentType(strContentType),
							total_time_(total),
							connect_time_(connect),
							namelookup_time_(namelookup_time)
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

void HttpResponse::GetErrDesc(std::string& strErrDesc)
{
	strErrDesc.swap(m_strErrDesc);
}

void HttpResponse::GetContentType(std::string& strContentType)
{
	strContentType.swap(m_strContentType);
}

void HttpResponse::GetHeader(std::string& strHeader)
{
	strHeader.swap(m_strHeader);
}

void HttpResponse::GetBody(std::string& strBody)
{
	strBody.swap(m_strBody);
}

void HttpResponse::GetTimeConsuming(double& total, double& connect, double& namelookup)
{
	total 		= total_time_;
	connect 	= connect_time_;
	namelookup 	= namelookup_time_;
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
	MaxTotalConns_(0),
	MaxHostConns_(0),	
	MaxConns_(0),
	MaxTotalWaitRsp_(10000),
	MaxHostWaitRsp_(100),
	IoThrNum_(4),
	MaxReqQueSize_(50000)
{
	total_req_ = 0;
}

CurlHttpClient::~CurlHttpClient()
{
	CurlManager::uninitialize();
}

void CurlHttpClient::DoneCallBack(const boost::any& _any, int errCode, const char* errDesc, double total_time, double connect_time, double namelookup_time)
{
	CurlRequestPtr clientPtr = boost::any_cast<CurlRequestPtr>(_any);
	//if (errCode != 0 || total_time > 0.5 || connect_time > 1.0)
	{
		LOG_INFO 
				 << "URL:" << clientPtr->getEffectiveUrl() << ",code:" << errCode << ",desc:" << errDesc 
				 //<< ",channel:" << clientPtr->getChannel() << ",requuid:" << clientPtr->getReqUUID()
			     << "\n" << "ResponseCode:" << clientPtr->getResponseCode() << " totaltm:" << total_time << " conntm:" << connect_time
			     //<< "\n" << clientPtr->getResponseHeader() << clientPtr->getResponseBody()
			     //<< "\n" << "bodysize:" << clientPtr->getResponseBody().size()
			     //<< "\n" << "ct:" << clientPtr->getResponseContentType()
			     //<< "\n" << "QueueLoopSize:" << clientPtr->getQueueLoopSize()
			     ;
	}
	HttpResponsePtr http_response(new HttpResponse(clientPtr->getResponseCode(), 
													errCode, 
													std::string(errDesc), 
													clientPtr->getResponseContentType(), 
													clientPtr->getResponseHeader(), 
													clientPtr->getResponseBody(),
													total_time,connect_time,namelookup_time
													)
								 );	

	if (m_pfunc_onmsg_cb)
	{
		m_pfunc_onmsg_cb(m_pInvoker, clientPtr->getReqUUID(), clientPtr->getContext(), http_response);
	}

	--wait_rsp_;
	
	/*
	--wait_rsp_;
	std::shared_ptr<CountDownLatch> latchPtr(boost::any_cast<std::shared_ptr<CountDownLatch>>(latch_));
	latchPtr->countDown();
	*/
	
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
		LOG_INFO << "SPEED:" << nSpeed/nTmDiff << " TOTAL:" << now_req << " WAIT:" << wait_rsp_.load();
	}
}

// IO线程池初始化成功回调函数
static void func_IoThrInit(const std::shared_ptr<CountDownLatch>& latch)
{
	latch->countDown();
}

bool CurlHttpClient::InitHttpClient(void* pInvoker,
									pfunc_onmsg_cb pFnOnMsg,
									bool enable_ssl,
									int nIoThrNum,
									int nMaxReqQueSize,
									int nMaxTotalWaitRsp,
									int nMaxHostWaitRsp,
									int nMaxTotalConns,
									int nMaxHostConns,	
									int nMaxConns)
{
	bool bRet = false;	

	// 创建IO线程池
	ThreadPoolPtr pIoThr(new ThreadPool("IoThr"));
	if (pIoThr)
	{
		m_pInvoker = pInvoker;
		m_pfunc_onmsg_cb = pFnOnMsg;
		MaxTotalConns_ = nMaxTotalConns;
		MaxHostConns_ = nMaxHostConns;
		MaxConns_ = nMaxConns;
		IoThrNum_ = nIoThrNum;
		MaxTotalWaitRsp_ = nMaxTotalWaitRsp;
		MaxHostWaitRsp_  = nMaxHostWaitRsp;
		MaxReqQueSize_   = nMaxReqQueSize;

		HttpRequesting::GetInstance().setMaxSize(MaxTotalWaitRsp_, MaxHostWaitRsp_);
		HttpWaitRequest::GetInstance().setMaxSize(MaxReqQueSize_);
		
		// 保存io_loop
		io_loop_ = pIoThr;

		// 启动线程池
		std::shared_ptr<CountDownLatch> latch(new CountDownLatch(nIoThrNum));
		latch_ = latch;
		pIoThr->setThreadInitCallback(std::bind(func_IoThrInit, latch));		
		pIoThr->start(nIoThrNum);
		latch->wait();
				
		// 初始化curl库
		CurlManager::initialize(enable_ssl?CurlManager::kCURLssl:CurlManager::kCURLnossl);
		
		// 将curl管理器跑在线程池里
		latch->resetCount(nIoThrNum);
		for (int i = 0; i < IoThrNum_; i++)
		{
			pIoThr->run(std::bind(&CurlHttpClient::Start, this));
		}
		latch->wait();
		
		bRet = true;
	}
	return bRet;
}					

void CurlHttpClient::Start()
{
	std::shared_ptr<CountDownLatch> latch(boost::any_cast<std::shared_ptr<CountDownLatch>>(latch_));
	std::shared_ptr<CurlManager> curlm(new CurlManager(MaxTotalConns_, MaxHostConns_, MaxConns_));
	httpclis_.push_back(curlm);
	curlm->runCurlEvLoop(latch);
}

//获取HttpRequest
HttpRequestPtr CurlHttpClient::GetHttpRequest(const std::string& strUrl, bool bKeepAlive, HTTP_REQUEST_TYPE req_type)
{
	// 生成curlrequest
	CurlRequestPtr curl_request = CurlManager::getRequest(strUrl, bKeepAlive, req_type);
	
	// 设置完成回调
	curl_request->setDoneCallback(std::bind(&CurlHttpClient::DoneCallBack,this,
																	std::placeholders::_1,
																	std::placeholders::_2,
																	std::placeholders::_3,
																	std::placeholders::_4,
																	std::placeholders::_5,
																	std::placeholders::_6));
	// 生成httprequest
	HttpRequestPtr http_request(new HttpRequest(strUrl, bKeepAlive, req_type));
	
	// 保存curl_request
	http_request->SetContext(curl_request);

	return http_request;
}

//发送http请求
int  CurlHttpClient::SendHttpRequest(const boost::any& params, const HttpRequestPtr& request)
{
	/*
	int n = (++wait_rsp_ - MaxTotalWaitRsp_);

	std::shared_ptr<CountDownLatch> latchPtr(boost::any_cast<std::shared_ptr<CountDownLatch>>(latch_));

	if (n > 0) latchPtr->addAndWait(n);
	*/
	
	// 取发送中队列未满的管理器
	uint64_t count = total_req_.load();
	uint64_t index = count % static_cast<uint64_t>(httpclis_.size());
	boost::any& _any = httpclis_[index];
	
	std::shared_ptr<CurlManager> curlm = boost::any_cast<std::shared_ptr<CurlManager>>(_any);

	int nRet = -1;
	
	CurlRequestPtr curl_req = boost::any_cast<CurlRequestPtr>(request->GetContext());
	if (curl_req)
	{		
		curl_req->setContext(params);
		nRet = curlm->sendRequest(curl_req);
	}

	if (0 == nRet) 
	{
		++total_req_;
		++wait_rsp_;
	}
	
	return nRet;
}

// 获取待回应数量
int  CurlHttpClient::GetWaitRspCnt()
{
	return wait_rsp_.load();
}

void CurlHttpClient::ExitHttpClient()
{
	CurlManager::uninitialize();
}

}
}
