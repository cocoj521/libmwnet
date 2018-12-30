#include "CurlRequest.h"
#include "CurlManager.h"
#include "CurlHttpClient.h"
#include <mwnet_mt/base/Logging.h>
#include <assert.h>

using namespace curl;

CurlRequest::CurlRequest(const std::string& url, uint64_t req_uuid, bool bKeepAlive, int req_type)
	: req_uuid_(req_uuid),
	  curl_(CHECK_NOTNULL(curl_easy_init())),
	  headers_(NULL),
	  requestHeaderStr_(""),
	  requestBodyStr_(""),
	  responseBodyStr_(""),
	  responseHeaderStr_(""),
	  keep_alive_(bKeepAlive),
	  req_type_(req_type),
	  url_(url)
{
	initCurlRequest(url, req_uuid, bKeepAlive, req_type);
}

void CurlRequest::initCurlRequest(const std::string& url, uint64_t req_uuid, bool bKeepAlive, int req_type)
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

	cleanHeaders();

	void* p = reinterpret_cast<void*>(req_uuid);

	// 设置请求模式为post/get
	if (MWNET_MT::CURL_HTTPCLIENT::HTTP_REQUEST_POST == req_type) curl_easy_setopt(curl_, CURLOPT_HTTPPOST, 1L);
	if (MWNET_MT::CURL_HTTPCLIENT::HTTP_REQUEST_GET  == req_type) curl_easy_setopt(curl_, CURLOPT_HTTPGET , 1L);
		
	// 设为不验证证书
	curl_easy_setopt(curl_, CURLOPT_SSL_VERIFYPEER, 0L);
	curl_easy_setopt(curl_, CURLOPT_SSL_VERIFYHOST, 0L);
	
	// 默认user-agent
	//curl_easy_setopt(curl_, CURLOPT_USERAGENT, curl_version());

	// Connection: Keep-Alive/Close
	setHeaders("Connection", keep_alive_?"Keep-Alive":"Close");

	if (!keep_alive_)
	{
		// 不复用已有连接，每次都申请新的
		curl_easy_setopt(curl_, CURLOPT_FRESH_CONNECT, 1L);
		// 不复用已有socket，每次都申请新的
		curl_easy_setopt(curl_, CURLOPT_FORBID_REUSE, 1L);
	}
	
	curl_easy_setopt(curl_, CURLOPT_WRITEFUNCTION, &CurlRequest::bodyDataCb);
	curl_easy_setopt(curl_, CURLOPT_WRITEDATA, this);
	
	curl_easy_setopt(curl_, CURLOPT_HEADERFUNCTION, &CurlRequest::headerDataCb);
	curl_easy_setopt(curl_, CURLOPT_HEADERDATA, this);
	
	curl_easy_setopt(curl_, CURLOPT_PRIVATE, p);
	//curl_easy_setopt(curl_, CURLOPT_PRIVATE, this);
	
	curl_easy_setopt(curl_, CURLOPT_NOSIGNAL, 1L);

	curl_easy_setopt(curl_, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_1_1);
	curl_easy_setopt(curl_, CURLOPT_COOKIESESSION, 1);

	curl_easy_setopt(curl_, CURLOPT_FOLLOWLOCATION, 1L);
	curl_easy_setopt(curl_, CURLOPT_AUTOREFERER, 1);

	curl_easy_setopt(curl_, CURLOPT_CAINFO, "./cacert.pem");
	
	// 接管curl的closesocket,socket的关闭交由httprequest结束时关闭channel时再做
	// 如果socket的关闭交由curl管理，由于时序问题，会造成fd复用，上层无法及时获取，所以由上层来关闭
	//curl_easy_setopt(curl_, CURLOPT_CLOSESOCKETFUNCTION, &CurlRequest::hookCloseSocket);
	//curl_easy_setopt(curl_, CURLOPT_CLOSESOCKETDATA, this);

	// socket的创建由上层完成
	//curl_easy_setopt(curl_, CURLOPT_OPENSOCKETFUNCTION, &CurlRequest::hookOpenSocket);
	//curl_easy_setopt(curl_, CURLOPT_OPENSOCKETDATA, this);

	//curl_easy_setopt(curl_, CURLOPT_NOPROGRESS, 0L);
	//curl_easy_setopt(curl_, CURLOPT_PROGRESSFUNCTION, prog_cb);

	//curl_easy_setopt(curl_, CURLOPT_FOLLOWLOCATION, 0L);
	//curl_easy_setopt(curl_, CURLOPT_AUTOREFERER, 1L);
	//curl_easy_setopt(curl_, CURLOPT_COOKIESESSION, 1L);

	curl_easy_setopt(curl_, CURLOPT_URL, url.c_str());
}

CurlRequest::~CurlRequest()
{
	// 释放headers_内存
	cleanHeaders();

	// 释放curl easy句柄
	cleanCurlHandles();
	
	LOG_DEBUG  << "CurlRequest: ~CurlRequest";
}

// 清理curl句柄
void CurlRequest::cleanCurlHandles()
{
	LOG_DEBUG << "CurlRequest::cleanCurlHandles";
	
	// 清除所有easy curl使用的地址空间
	curl_easy_cleanup(curl_);
}

int CurlRequest::hookOpenSocket(void *clientp, curlsocktype purpose, struct curl_sockaddr *address)
{
	int fd = -1;
	CurlRequest* p = static_cast<CurlRequest*>(clientp);
	if (p)
	{
		fd = ::socket(address->family, address->socktype, address->protocol);
	}
	//LOG_DEBUG << "hookOpenSocket:" << fd << "--loop:" << p->loop_ << "--channel:" << p->getChannel() << "--curl:" << p->curl_ << "--uuid:" << p->req_uuid_;
	return fd;
}

int CurlRequest::hookCloseSocket(void *clientp, int fd)
{
	LOG_DEBUG << "CurlRequest::hookCloseSocket:" << fd;
	CurlRequest* p = static_cast<CurlRequest*>(clientp);
	if (p)
	{
		//p->loop_->runInLoop(std::bind(&CurlRequest::closeChannelFd, p, p->channel_));
	}
	return 0;
}

void CurlRequest::setTimeOut(long conn_timeout, long entire_timeout)
{
	// 设置entire超时时间
	curl_easy_setopt(curl_, CURLOPT_TIMEOUT, entire_timeout);
	// 设置connect超时时间
	curl_easy_setopt(curl_, CURLOPT_CONNECTTIMEOUT, conn_timeout);
}

void CurlRequest::request(CURLM* multi)
{
	// 将头部信息加入curl
	setHeaders();

	// 加入事件管理器中，响应事件
	curl_multi_add_handle(multi, curl_);

	LOG_DEBUG << "CurlRequest::request";
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
	LOG_DEBUG << "CurlRequest::removeMultiHandle";
	
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

const char* CurlRequest::getEffectiveUrl()
{
	const char* p = NULL;
	curl_easy_getinfo(curl_, CURLINFO_EFFECTIVE_URL, &p);
	return p;
}

const char* CurlRequest::getRedirectUrl()
{
	const char* p = NULL;
	curl_easy_getinfo(curl_, CURLINFO_REDIRECT_URL, &p);
	return p;
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
	double total_time;
	curl_easy_getinfo(curl_, CURLINFO_TOTAL_TIME, &total_time);
	LOG_DEBUG << "TOTAL TIME:" << total_time*1000 << "ms\n";

	double connect_time;
	curl_easy_getinfo(curl_, CURLINFO_CONNECT_TIME, &connect_time);
	LOG_DEBUG << "CONNECT TIME:" << connect_time*1000 << "ms\n";

	double nameloopup_time;
	curl_easy_getinfo(curl_, CURLINFO_NAMELOOKUP_TIME, &nameloopup_time);
	LOG_DEBUG << "NAMELOOPUP TIME:" << nameloopup_time*1000 << "ms\n";

	if (doneCb_)
	{
		doneCb_(shared_from_this(), errCode, errDesc, total_time, connect_time, nameloopup_time);
	}
	LOG_DEBUG << "CurlRequest::done";
}

void CurlRequest::responseBodyCallback(const char* buffer, int len)
{
	LOG_DEBUG << "responseBodyCallback-len:" << len << " data:" << buffer;

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
