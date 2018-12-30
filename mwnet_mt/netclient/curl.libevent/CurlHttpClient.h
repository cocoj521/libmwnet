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

class HttpRequest
{
public:
	HttpRequest(const std::string& strUrl,      /*
												请求url,如果是get请求,则是包含参数在内的完整的请求;
											    如果是get请求,则是请求url,如:http://ip/domain:port/v1/func
											    */
				   bool bKeepAlive=false, 	    /*是否保持长连接,是:Connection自动追加Keep-Alive,否:自动追加Close*/
				   HTTP_REQUEST_TYPE req_type=HTTP_REQUEST_POST);/*指定是发post还是get请求*/
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
};

class HttpResponse
{
public:
	HttpResponse(int nRspCode,int nErrCode,
					const std::string& strErrDesc, 
					const std::string& strContentType, 
					std::string& strHeader, 
					std::string& strBody, 
					double total, double connect, double namelookup_time);
	~HttpResponse();
public:
	//获取状态码，200，400，404 etc.
	int    GetRspCode() const;

	//获取错误代码
	int	   GetErrCode() const;

	//获取错误描述
	void   GetErrDesc(std::string& strErrDesc);
	
	//获取conetnettype，如：text/json，application/x-www-form-urlencoded，text/xml
	void   GetContentType(std::string& strContentType);

	//获取回应包的包头
	void   GetHeader(std::string& strHeader);

	//获取回应包的包体
	void   GetBody(std::string& strBody);

	//获取耗时
	void   GetTimeConsuming(double& total, double& connect, double& namelookup);
private:
	int m_nRspCode;
	int m_nErrCode;
	std::string m_strErrDesc;
	std::string m_strContentType;
	std::string m_strHeader;
	std::string m_strBody;
	double total_time_;
	double connect_time_;
	double namelookup_time_;
	//接收时间戳
	//.....
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
						int nMaxReqQueSize=50000,   /*请求队列最大值,超过该值会被限速*/
						int nMaxTotalWaitRsp=1000,	/*最大容许待回应的总数量,超过该数量总体将被限速*/
						int nMaxHostWaitRsp=100,	/*单个host最大容许待回应的数量,超过该数量该host将被限速*/
						int nMaxTotalConns=0,		/*curl multi参数：CURLMOPT_MAX_TOTAL_CONNECTIONS 暂取默认值,无需设置*/
						int nMaxHostConns=0,		/*curl multi参数：CURLMOPT_MAX_HOST_CONNECTIONS 暂取默认值,无需设置*/	
						int nMaxConns=0);			/*curl multi参数：CURLMOPT_MAXCONNECTS 暂取默认值,无需设置*/

	//获取HttpRequest
	HttpRequestPtr GetHttpRequest(const std::string& strUrl,     	/*
																	请求url,如果是get请求,则是包含参数在内的完整的请求;
											    					如果是get请求,则是请求url,如:http://ip/domain:port/v1/func
											    					*/
				   					bool bKeepAlive=false, 	   	    /*是否保持长连接,是:Connection自动追加Keep-Alive,否:自动追加Close*/
				   					HTTP_REQUEST_TYPE req_type=HTTP_REQUEST_POST);/*指定是发post还是get请求*/

	//发送http请求
	int  SendHttpRequest(const boost::any& params, 			/*每次请求携带的任意数据,回调返回时会返回*/
							const HttpRequestPtr& request);	/*完整的http请求数据,每次请求前调用GetHttpRequest,并填充*/
	
	// 获取待回应数量
	int  GetWaitRspCnt();

	void ExitHttpClient();
private:
	void Start();
	void DoneCallBack(const boost::any& _any, int errCode, const char* errDesc, double total_time, double connect_time, double namelookup_time);	
private:
	void* m_pInvoker;
	// 回调函数
	pfunc_onmsg_cb m_pfunc_onmsg_cb;
	// mainloop
	//boost::any main_loop_;
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
	int MaxConns_;
	int MaxTotalWaitRsp_;
	int MaxHostWaitRsp_;
	int IoThrNum_;
	int MaxReqQueSize_;
};

}
}

#endif
