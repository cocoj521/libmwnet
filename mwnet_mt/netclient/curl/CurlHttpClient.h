#ifndef MWNET_MT_CURL_HTTPCLIENT_H
#define MWNET_MT_CURL_HTTPCLIENT_H

#include <stdint.h>
#include <stdio.h>
#include <boost/any.hpp>
#include <string>
#include <memory>
#include <atomic>

namespace MWNET_MT
{
namespace CURL_HTTPCLIENT
{
//content-type
enum HTTP_CONTENT_TYPE { CONTENT_TYPE_XML, CONTENT_TYPE_JSON, CONTENT_TYPE_URLENCODE, CONTENT_TYPE_UNKNOWN };
//POST/GET ...
enum HTTP_REQUEST_TYPE { HTTP_REQUEST_POST, HTTP_REQUEST_GET, HTTP_REQUEST_UNKNOWN };
//HTTP VERSION
enum HTTP_VERSION { HTTP_1_0, HTTP_1_1, HTTP_2_0 };

class HttpRequest
{
public:
	HttpRequest(const std::string& strUrl,						/*
																请求url,如果是get请求,则是包含参数在内的完整的请求;
																如果是POST请求,则是请求url,如:http://ip/domain:port/v1/func
																*/
			    HTTP_VERSION http_ver=HTTP_1_1,	 				/*http请求版本,1.0,1.1,2.0 etc.*/
				HTTP_REQUEST_TYPE req_type=HTTP_REQUEST_POST,	/*请求方式:post/get*/
				bool bKeepAlive=false							/*长连接/短连接*/
				);
	~HttpRequest();
public:

	//设置HTTP请求body数据
	//内部用到了swap函数，所以参数没有加const
	//返回值:void
	void	SetBody(std::string& strBody);

	//设置请求头 
	//如有多个头域需要设置，调用多次即可
	//返回值:void
	void	SetHeader(const std::string& strField, const std::string& strValue);
	
	//设置User-Agent  
	//如不设置，自动取默认值，默认值为:curl版本号(curl_version()函数的返回值)
	//也可以使用SetHeader设置
	//返回值:void
	void	SetUserAgent(const std::string& strUserAgent);

	//设置content-type
	//也可以使用SetHeader设置
	void	SetContentType(const std::string& strContentType);

	//设置超时时间
	//conn_timeout:连接超时时间
	//entire_timeout:整个请求的超时间
	void	SetTimeOut(int conn_timeout=2, int entire_timeout=5);

	//设置keepalive空闲维持时间和保活探针间隔时间(单位秒)
	void 	SetKeepAliveTime(int keep_idle=30, int keep_intvl=10);

	//设置dns域名解析缓存超时时间,若不指定具体的时间,默认同一域名60s内只解析一次
	void 	SetDnsCacheTimeOut(int cache_timeout=60);
	
	//获取本次请求的惟一req_uuid
	uint64_t GetReqUUID() const;
	
///////////////////////////////////////////////////////////////////////////////////////
	//两个内部函数,勿用！
	///////////////////////////////////////////////////
	void	SetContext(const boost::any& context);
	const boost::any& GetContext() const;
///////////////////////////////////////////////////
private:
	boost::any any_;
	bool keep_alive_;
	HTTP_REQUEST_TYPE req_type_;
	HTTP_VERSION http_ver_;
};

class HttpResponse
{
public:
	HttpResponse(int nRspCode,int nErrCode,
					const std::string& strErrDesc, 
					const std::string& strContentType, 
					std::string& strHeader, 
					std::string& strBody, 
					const std::string& strEffectiveUrl,
					const std::string& strRedirectUrl,
					uint32_t total,uint32_t connect,uint32_t namelookup_time,
					uint64_t rsp_time,uint64_t req_time,uint64_t req_inque_time,uint64_t req_uuid);
	~HttpResponse();
public:
	//获取状态码，200，400，404 etc.
	int    GetRspCode() const;

	//获取错误代码
	int	   GetErrCode() const;

	//获取错误描述
	const std::string& GetErrDesc() const;
	
	//获取conetnettype，如：text/json，application/x-www-form-urlencoded，text/xml
	const std::string& GetContentType() const;

	//获取回应包的包头
	const std::string& GetHeader() const;

	//获取回应包的包体
	const std::string& GetBody() const;

	//获取回应对应的请求的有效URL地址
	const std::string& GetEffectiveUrl() const;

	//获取回应对应的请求的重定向URL地址
	const std::string& GetRedirectUrl() const;

	//获取耗时(单位:微秒)
	//total:总耗时,connect:连接耗时,nameloopup:域名解析耗时
	void   GetTimeConsuming(uint32_t& total, uint32_t& connect, uint32_t& namelookup) const;

	//获取回应对应的请求整体耗时(单位:微秒)
	uint32_t   GetReqTotalConsuming() const;

	//获取回应对应的请求连接耗时(单位:微秒)
	uint32_t   GetReqConnectConsuming() const;

	//获取回应对应的请求域名解析耗时(单位:微秒)
	uint32_t   GetReqNameloopupConsuming() const;
	
	//获取回应返回的时间(精确到微秒)
	uint64_t   GetRspRecvTime() const;

	//获取该回应对应的请求入队时间(精确到微秒)
	uint64_t   GetReqInQueTime() const;

	//获取该回应对应的请求的请求发出时间(精确到微秒)
	uint64_t   GetReqSendTime() const;

	//获取该回应对应的请求UUID
	uint64_t   GetReqUUID() const;
private:
	int m_nRspCode;
	int m_nErrCode;
	std::string m_strErrDesc;
	std::string m_strContentType;
	std::string m_strHeader;
	std::string m_strBody;
	std::string m_strEffectiveUrl;
	std::string m_strRedirectUrl;
	uint32_t total_time_;
	uint32_t connect_time_;
	uint32_t namelookup_time_;
	uint64_t rsp_time_;
	uint64_t req_time_;
	uint64_t req_inque_time_;
	uint64_t req_uuid_;
};

typedef std::shared_ptr<HttpRequest> HttpRequestPtr;
typedef std::shared_ptr<HttpResponse> HttpResponsePtr;

/**************************************************************************************
* Function      : pfunc_onmsg_cb
* Description   : 回调函数：与回应相关的事件回调(消息回应、超时)
* Input         : [IN] void* pInvoker						上层调用的类指针
* Input         : [IN] uint64_t req_uuid					SendHttpRequest函数的参数HttpRequestPtr的成员函数GetReqUUID的值
* Input         : [IN] const boost::any& params				SendHttpRequest函数的参数params
* Input         : [IN] const HttpResponsePtr& response		HttpResponse响应类
**************************************************************************************/
typedef int(*pfunc_onmsg_cb)(void* pInvoker, uint64_t req_uuid, const boost::any& params, const HttpResponsePtr& response);

class CurlHttpClient
{
public:
	CurlHttpClient();
	~CurlHttpClient();

public:
	//初始化函数，在curlhttpclient生命周期内，只可调用一次
	bool InitHttpClient(void* pInvoker,				/*上层调用的类指针*/
						pfunc_onmsg_cb pFnOnMsg, 	/*与回应相关的事件回调(recv)*/
						bool enable_ssl=false,		/*是否加载ssl.如不需要https,可以不加载*/
						int nIoThrNum=4,			/*IO工作线程数*/
						int nMaxReqQueSize=20000,   /*请求队列中允许缓存的待请求数量,超过该值会返回失败*/
						int nMaxTotalConns=10000,	/*总的最大允许的并发请求数，超过该数量再请求会返回失败*/	
						int nMaxHostConns=20		/*单个host最大允许的并发请求数，超过该数量再请求会返回失败*/
						);

	//获取HttpRequest
	HttpRequestPtr GetHttpRequest(const std::string& strUrl,     					/*
																					请求url,如果是get请求,则是包含参数在内的完整的请求;
															    					如果是POST请求,则是请求url,如:http://ip/domain:port/v1/func
															    					*/
									HTTP_VERSION http_ver=HTTP_1_1,	 				/*http请求版本,1.0,1.1,2.0 etc.*/
									HTTP_REQUEST_TYPE req_type=HTTP_REQUEST_POST,	/*请求方式:post/get*/
									bool bKeepAlive=false							/*长连接/短连接*/
									);

	//发送http请求
	int  SendHttpRequest(const boost::any& params, 			/*每次请求携带的任意数据,回调返回时会返回*/
							const HttpRequestPtr& request);	/*完整的http请求数据,每次请求前调用GetHttpRequest,并填充*/
	
	// 获取待回应数量
	int  GetWaitRspCnt();

	void ExitHttpClient();
private:
	void Start();
	void DoneCallBack(const boost::any& _any, int errCode, const char* errDesc);	
private:
	void* m_pInvoker;
	// 回调函数
	pfunc_onmsg_cb m_pfunc_onmsg_cb;
	// mainloop
	boost::any main_loop_;
	// ioloop
	boost::any io_loop_;
	// latch
	boost::any latch_;
	// httpclis
	std::vector<boost::any> httpclis_;
	// 总请求数
	std::atomic<uint64_t> total_req_;
	// 待回应的数量
	std::atomic<int> wait_rsp_;
	int MaxTotalConns_;
	int MaxHostConns_;
	int IoThrNum_;
	int MaxReqQueSize_;
};

}
}

#endif
