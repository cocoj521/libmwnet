#include "CurlRequest.h"
#include "CurlManager.h"
#include "CurlHttpClient.h"
#include "RequestManager.h"
#include <mwnet_mt/base/Logging.h>
#include <assert.h>

using namespace curl;

CurlRequest::CurlRequest(const std::string& url, uint64_t req_uuid, bool bKeepAlive, int req_type, int http_ver)
	: req_uuid_(req_uuid),
	  curl_(CHECK_NOTNULL(curl_easy_init())),
	  headers_(NULL),
	  requestHeaderStr_(""),
	  requestBodyStr_(""),
	  responseBodyStr_(""),
	  responseHeaderStr_(""),
	  keep_alive_(bKeepAlive),
	  req_type_(req_type),
	  url_(url),
	  http_ver_(http_ver),
	  entire_timeout_(60)
{
	initCurlRequest(url, req_uuid, bKeepAlive, req_type, http_ver);
	LOG_DEBUG << "curl_new";
}

void CurlRequest::initCurlRequest(const std::string& url, uint64_t req_uuid, bool bKeepAlive, int req_type, int http_ver)
{
	LOG_DEBUG << "InitCurlRequest";

	req_uuid_ = req_uuid;
	requestHeaderStr_ = "";
	requestBodyStr_ = "";
	responseBodyStr_ = "";
	responseHeaderStr_ = "";
	keep_alive_ = bKeepAlive;
	req_type_ = req_type,
	url_ = url;
	http_ver_ = http_ver;

	total_time_ = 0;
	connect_time_ = 0;
	namelookup_time_ = 0;
	rsp_time_ = 0;
	req_time_ = 0;
	req_inque_time_ = 0;
	
	cleanHeaders();

	void* p = reinterpret_cast<void*>(req_uuid);

	// 设置请求模式为post/get
	if (MWNET_MT::CURL_HTTPCLIENT::HTTP_REQUEST_POST == req_type) curl_easy_setopt(curl_, CURLOPT_HTTPPOST, 1L);
	if (MWNET_MT::CURL_HTTPCLIENT::HTTP_REQUEST_GET  == req_type) curl_easy_setopt(curl_, CURLOPT_HTTPGET , 1L);

	// 设置http版本
	switch (http_ver_)
	{
		case MWNET_MT::CURL_HTTPCLIENT::HTTP_1_0:
			{
				curl_easy_setopt(curl_, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_1_0);
				break;
			}
		case MWNET_MT::CURL_HTTPCLIENT::HTTP_1_1:
			{
				curl_easy_setopt(curl_, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_1_1);
				break;
			}
		case MWNET_MT::CURL_HTTPCLIENT::HTTP_2_0:
			{
				curl_easy_setopt(curl_, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_2_0);
				break;
			}
		default:
			break;
	}
	
	// 设为不验证证书
	curl_easy_setopt(curl_, CURLOPT_SSL_VERIFYPEER, 0L);
	curl_easy_setopt(curl_, CURLOPT_SSL_VERIFYHOST, 0L);
	curl_easy_setopt(curl_, CURLOPT_CAINFO, "./notfound.pem");
	
	// 默认user-agent
	curl_easy_setopt(curl_, CURLOPT_USERAGENT, curl_version());

	//关闭Expect: 100-continue
	setHeaders("Expect", "");

	// Connection: Keep-Alive/Close
	setHeaders("Connection", keep_alive_?"Keep-Alive":"Close");

	if (!keep_alive_)
	{
		// 不复用已有连接，每次都申请新的
		curl_easy_setopt(curl_, CURLOPT_FRESH_CONNECT, 1L);
		// 不复用已有socket，每次都申请新的
		curl_easy_setopt(curl_, CURLOPT_FORBID_REUSE, 1L);
	}
	else
	{
		curl_easy_setopt(curl_, CURLOPT_TCP_KEEPALIVE, 1L);
		curl_easy_setopt(curl_, CURLOPT_TCP_KEEPIDLE, 60L);
		curl_easy_setopt(curl_, CURLOPT_TCP_KEEPINTVL, 3L);

		//Limit the age(idle time) of connections for reuse.See CURLOPT_MAXAGE_CONN
		//curl_easy_setopt(curl_, CURLOPT_MAXAGE_CONN, 30L);
		//Limit the age(since creation) of connections for reuse.See CURLOPT_MAXLIFETIME_CONN
		//curl_easy_setopt(curl_, CURLOPT_MAXLIFETIME_CONN, 30L);
	}
	
	curl_easy_setopt(curl_, CURLOPT_WRITEFUNCTION, &CurlRequest::bodyDataCb);
	curl_easy_setopt(curl_, CURLOPT_WRITEDATA, this);
	
	curl_easy_setopt(curl_, CURLOPT_HEADERFUNCTION, &CurlRequest::headerDataCb);
	curl_easy_setopt(curl_, CURLOPT_HEADERDATA, this);
	
	curl_easy_setopt(curl_, CURLOPT_PRIVATE, p);
	//curl_easy_setopt(curl_, CURLOPT_PRIVATE, this);

	// 允许重定向
	curl_easy_setopt(curl_, CURLOPT_FOLLOWLOCATION, 1L);
	// maximum number of redirects allowed
	curl_easy_setopt(curl_, CURLOPT_MAXREDIRS, 2L);

	curl_easy_setopt(curl_, CURLOPT_NOSIGNAL, 1L);
	
	curl_easy_setopt(curl_, CURLOPT_URL, url.c_str());
}

CurlRequest::~CurlRequest()
{
	// 释放headers_内存
	cleanHeaders();

	// 释放curl easy句柄
	cleanCurlHandles();
	
	LOG_DEBUG  << "curl_del";
}

// 清理curl句柄
void CurlRequest::cleanCurlHandles()
{
	LOG_DEBUG << "curl = " << curl_;
	
	// 清除所有easy curl使用的地址空间
	curl_easy_cleanup(curl_);
}

void CurlRequest::setTimeOut(long conn_timeout, long entire_timeout)
{
	entire_timeout_ = entire_timeout;
	// 设置entire超时时间
	curl_easy_setopt(curl_, CURLOPT_TIMEOUT, entire_timeout);
	// 设置connect超时时间
	curl_easy_setopt(curl_, CURLOPT_CONNECTTIMEOUT, conn_timeout);
}

void CurlRequest::setKeepAliveTime(long keep_idle, long keep_intvl)
{
	// keep alive空闲时间
	curl_easy_setopt(curl_, CURLOPT_TCP_KEEPIDLE, keep_idle);
	// 保活探针间隔时间
	curl_easy_setopt(curl_, CURLOPT_TCP_KEEPINTVL, keep_intvl);
}

// 取消请求
void CurlRequest::forceCancel()
{
	/*
	LOG_DEBUG << "req_uuid = " << req_uuid_;
	loop_->queueInLoop(std::bind(&CurlManager::forceCancelRequest, cm_, req_uuid_));
	*/
}

// 强制取消请求(内部调用)
void CurlRequest::forceCancelInner()
{
	/*
	LOG_DEBUG << "req_uuid = " << req_uuid_;
	loop_->queueInLoop(std::bind(&CurlManager::forceCancelRequestInner, cm_, shared_from_this()));
	*/
}

void CurlRequest::request(CURLM* multi)
{
	// 将头部信息加入curl
	setHeaders();

	req_time_ = Timestamp::GetCurrentTimeUs();
	
	// 加入事件管理器中，响应事件
	curl_multi_add_handle(multi, curl_);

	LOG_DEBUG << "multi = " << multi << " curl = " << curl_;
}

// 内部使用
void CurlRequest::setHeaders()
{
	// 设置头部信息
	curl_easy_setopt(curl_, CURLOPT_HTTPHEADER, headers_);
}

// 内部使用
void CurlRequest::cleanHeaders()
{	
	if (headers_ != NULL)
	{
		// 清理头指针信息
		curl_slist_free_all(headers_);
		headers_ = NULL;
	}
}

void CurlRequest::removeMultiHandle(CURLM* multi)
{		
	LOG_DEBUG << "multi = " << multi;
	
	// 将curl从事件管理器中移除，之后将不再响应事件
	curl_multi_remove_handle(multi, curl_);
}

void CurlRequest::headerOnly()
{
	curl_easy_setopt(curl_, CURLOPT_NOBODY, 1);
}

void CurlRequest::setRange(const std::string& range)
{
	 curl_easy_setopt(curl_, CURLOPT_RANGE, range.c_str());
}

std::string CurlRequest::getEffectiveUrl()
{
	char *p = NULL;
    CURLcode rc = curl_easy_getinfo(curl_, CURLINFO_EFFECTIVE_URL, &p);
    if(!rc && p) 
	{
		return std::string(p);
	}
	else
	{	
		return "";
	}
}

std::string CurlRequest::getRedirectUrl()
{
	char *p = NULL;
    CURLcode rc = curl_easy_getinfo(curl_, CURLINFO_REDIRECT_URL, &p);
    if(!rc && p) 
	{
		return std::string(p);
	}
	else
	{	
		return "";
	}
}

std::string& CurlRequest::getResponseHeader()
{
	return responseHeaderStr_;
}

std::string& CurlRequest::getResponseBody()
{
	return responseBodyStr_;
}

int CurlRequest::getResponseCode()
{
	long code = 0;
	curl_easy_getinfo(curl_, CURLINFO_RESPONSE_CODE, &code);
	return static_cast<int>(code);
}

std::string CurlRequest::getResponseContentType()
{
	char *ct = NULL;
    CURLcode rc = curl_easy_getinfo(curl_, CURLINFO_CONTENT_TYPE, &ct);
    if(!rc && ct) 
	{
		return std::string(ct);
	}
	else
	{	
		return "";
	}
}

void CurlRequest::done(int errCode, const char* errDesc)
{
	rsp_time_ = Timestamp::GetCurrentTimeUs();
	
	double total_time;
	curl_easy_getinfo(curl_, CURLINFO_TOTAL_TIME, &total_time);
	
	double connect_time;
	curl_easy_getinfo(curl_, CURLINFO_CONNECT_TIME, &connect_time);
	
	double nameloopup_time;
	curl_easy_getinfo(curl_, CURLINFO_NAMELOOKUP_TIME, &nameloopup_time);

	total_time_ = static_cast<int>(total_time * 1000000);
	connect_time_ = static_cast<int>(connect_time * 1000000);
	namelookup_time_ = static_cast<int>(nameloopup_time * 1000000);

	LOG_DEBUG << "TOTAL TIME:" << total_time_ << "us"
		<< " CONNECT TIME:" << connect_time_ << "us"
		<< " NAMELOOPUP TIME:" << namelookup_time_ << "us"
		<< " errCode = " << errCode << " errDesc = " << errDesc;

	if (doneCb_)
	{
		doneCb_(shared_from_this(), errCode, errDesc);
	}
}

void CurlRequest::responseBodyCallback(const char* buffer, int len)
{
	LOG_DEBUG << "len = " << len << " data = " << buffer;

	responseBodyStr_.append(buffer, len);
	
	if (bodyCb_)
	{
		bodyCb_(shared_from_this(), buffer, len);
	}
}

void CurlRequest::responseHeaderCallback(const char* buffer, int len)
{
	responseHeaderStr_.append(buffer, len);
	
	if (headerCb_)
	{
		headerCb_(shared_from_this(), buffer, len);
	}
}

size_t CurlRequest::bodyDataCb(char* buffer, size_t size, size_t nmemb, void* userp)
{
	CurlRequest* req = static_cast<CurlRequest*>(userp);
	if (req != NULL)
	{
		req->responseBodyCallback(buffer, static_cast<int>(nmemb*size));
		return nmemb*size;
	}
	else
	{
		return 0;
	}
}

size_t CurlRequest::headerDataCb(char* buffer, size_t size, size_t nmemb, void* userp)
{
	CurlRequest* req = static_cast<CurlRequest*>(userp);
	if (req != NULL)
	{
		req->responseHeaderCallback(buffer, static_cast<int>(nmemb*size));
		return nmemb*size;
	}
	else
	{	
		return 0;
	}
}

void CurlRequest::setUserAgent(const std::string& strUserAgent)
{
	curl_easy_setopt(curl_, CURLOPT_USERAGENT, strUserAgent.c_str());
}

void CurlRequest::setBody(std::string& strBody)
{
	requestBodyStr_.swap(strBody);
	curl_easy_setopt(curl_, CURLOPT_POSTFIELDSIZE, requestBodyStr_.size());
	curl_easy_setopt(curl_, CURLOPT_POSTFIELDS, requestBodyStr_.c_str());
}

void CurlRequest::setHeaders(const std::string& field, const std::string& value)
{
	std::string strLine = "";
	strLine.append(field);
	strLine.append(": ");
	strLine.append(value);
	headers_ = curl_slist_append(headers_, strLine.c_str());
}
