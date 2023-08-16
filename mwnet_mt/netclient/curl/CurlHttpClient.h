#ifndef MWNET_MT_CURL_HTTPCLIENT_H
#define MWNET_MT_CURL_HTTPCLIENT_H

#include <stdint.h>
#include <stdio.h>
#include <boost/any.hpp>
#include <string>
#include <memory>
#include <atomic>
#include <vector>

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

	// 设置https校验对端、校验host、证书地址(暂不生效。默认不校验对端，不校验host，无需证书)
	void 	SetHttpsVerifyPeer();
	void 	SetHttpsVerifyHost();
	void	SetHttpsCAinfo(const std::string& strCAinfo/*证书地址*/);

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
	// 0:成功
	// curl的错误码为1~93 
	// 库内部有定时器会检测超时，以防curl的超时失效，超时意义与curl的相似，在curl错误码的基础上增加了10000，以示区分
	// 10028:当内部定时检测到超时时间已到但curl仍未超时回调时会返回10028
	// 10055:当程序退出时会将所有未发送的请求全部回调，并返回10055，表示发送失败
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

	//获取耗时(单位:毫秒)
	//total:总耗时,connect:连接耗时,nameloopup:域名解析耗时
	void   GetTimeConsuming(uint32_t& total, uint32_t& connect, uint32_t& namelookup) const;

	//获取回应对应的请求整体耗时(单位:毫秒)
	uint32_t   GetReqTotalConsuming() const;

	//获取回应对应的请求连接耗时(单位:毫秒)
	uint32_t   GetReqConnectConsuming() const;

	//获取回应对应的请求域名解析耗时(单位:毫秒)
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
						int nIoThrNum=4,			/*IO工作线程数*/
						int nMaxReqQueSize=10000,   /*请求队列中允许缓存的待请求数量(应小于等于nMaxTotalConns，若超过将取nMaxTotalConns),超过该值会返回失败*/
						int nMaxTotalConns=10000,	/*总的最大允许的并发请求数(一台服务器最多65535，请设置<=60000的值)，超过该数量再请求会返回失败*/	
						int nMaxHostConns=5			/*单个host最大允许的并发请求数（暂不生效）*/
						);

	//获取HttpRequest
	HttpRequestPtr GetHttpRequest(const std::string& strUrl,     					/*
																					请求url,如果是get请求,则是包含参数在内的完整的请求;
															    					如果是POST请求,则是请求url,如:http://ip/domain:port/v1/func
															    					*/
									HTTP_VERSION http_ver=HTTP_1_1,	 				/*http请求版本,1.0,1.1,2.0 etc.*/
									HTTP_REQUEST_TYPE req_type=HTTP_REQUEST_POST,	/*请求方式:post/get*/
									bool bKeepAlive=false							/*长连接/短连接,暂只支持一个地址的长连接,若需要往多个地址上发送请求,不可以使用长连接*/
									);

	//发送http请求
	//0:成功 1:尚未初始化 2:发送队列满(超过了nMaxReqQueSize) 3:超过总的最大并发数(超过了nMaxTotalConns) 
	//!!!注意:调用该接口的线程数必须小于InitHttpClient接口中nMaxTotalConns的值,否则可能分出现永久阻塞
	int  SendHttpRequest(const HttpRequestPtr& request,	 /*完整的http请求数据,每次请求前调用GetHttpRequest获取,然后并填充所需参数*/
						const boost::any& params,		 /*每次请求可携带一个任意数据,回调返回时会返回,建议填写智能指针*/
						bool WaitOrReturnIfOverMaxTotalConns=false); /*超过总的最大并发数后，选择阻塞等待还是直接返回失败*/ 
																	 /*false:直接失败 true:阻塞等待并发减少后再发送*/	 
	
	// 取消请求，当请求未发送或已发送但还没回应时有效，会通过回调函数返回--暂未实现
	void CancelHttpRequest(const HttpRequestPtr& request);

	// 获取待回应数量
	size_t  GetWaitRspCnt() const;

	// 获取待发请求数量
	size_t  GetWaitReqCnt() const;

	// 退出
	void ExitHttpClient();
private:
	void Start();
	void DoneCallBack(const boost::any& _any, int errCode, const char* errDesc);	
private:
	void* m_pInvoker;
	// 回调函数
	pfunc_onmsg_cb m_pfunc_onmsg_cb;
	// mainloop
	// ioloop
	boost::any io_loop_;
	// latch
	boost::any latch_;
	// condition
	boost::any condition_;
	// mutex
	boost::any mutexLock_;
	// httpclis
	std::vector<boost::any> httpclis_;
	// 总请求数
	std::atomic<uint64_t> total_req_;
	// 待回应的数量
	std::atomic<int> wait_rsp_;
	// 最大并发连接数
	int MaxTotalConns_;
	// 单host最大并发连接数
	int MaxHostConns_;
	// io线程数
	int IoThrNum_;
	// 请求队列缓存的最大数量
	int MaxReqQueSize_;
	// 退出标记
	bool exit_;
};

}
}

#endif
