#ifndef MWNET_MT_NETCLIENT_H
#define MWNET_MT_NETCLIENT_H

#include <stdint.h>
#include <stdio.h>
#include <boost/any.hpp>
#include <string>
#include <sstream>
#include <memory>
#include <mutex>
#include <utility>
#include <stdio.h>
#include <stdarg.h>
#include <unistd.h>
#include <vector>
#include <list>

namespace MWNET_MT
{
namespace CLIENT
{
//content-type
enum HTTP_CONTENT_TYPE { CONTENT_TYPE_XML=1, CONTENT_TYPE_JSON, CONTENT_TYPE_URLENCODE, CONTENT_TYPE_UNKNOWN };
//POST/GET ...
enum HTTP_REQUEST_TYPE { HTTP_REQUEST_POST=1, HTTP_REQUEST_GET, HTTP_REQUEST_UNKNOWN };

class HttpRequest
{
public:
	HttpRequest();
	HttpRequest(const std::string& strReqIp, int nReqPort = 80);
	~HttpRequest();
public:
	//是否需要保持连接,即connection是keep-alive还是close
	//返回值:void
	void	SetKeepAlive(bool bKeepLive);

	bool    GetKeepAlive() const;

	//设置post请求数据
	//返回值:void
	void	SetContent(const std::string& strContent);

	//设置请求url
	//void
	//说明:如:post请求,POST /sms/std/single_send HTTP/1.1 ,/sms/std/single_send就是URL
	//	   如:get请求,
	void	SetUrl(const std::string& strUrl);

	//设置请求客户端的IP
	//返回值:请求客户端的IP
	void	SetRequestIp(const std::string& strIp);

	//设置请求客户端的端口
	//返回值:void
	void	SetRequestPort(int nPort);

	//设置User-Agent
	//返回值:void
	void	SetUserAgent(const std::string& strUserAgent);

	//设置content-type
	//返回值:CONTENT_TYPE_XML 代表 xml,CONTENT_TYPE_JSON 代表 json,CONTENT_TYPE_URLENCODE 代表 urlencode,CONTENT_TYPE_UNKNOWN 代表 未知类型
	//说明:暂只提供这三种
	void	SetContentType(int nContentType);

	//设置host
	//void
	void	SetHost(const std::string& strHost);

	//设置http请求的类型
	//值:HTTP_REQUEST_POST 代表 post请求; HTTP_REQUEST_GET 代表 get请求 ;
	void	SetRequestType(int nRequestType);

	int		GetTimeOut()const;

	//设置超时
	void	SetTimeOut(int nTimeOut);

public:
	//获取请求客户端的IP
	//返回值:void
	void GetRequestIp(std::string& strIp) const;

	//获取请求客户端的端口
	//返回值:int
	int	GetRequestPort(void) const;

	void GetRequestStr(std::string& strRequestStr) const;

	void GetContentTypeStr(int nContentType, std::string& strContentType) const;

private:
	std::string m_strIp;
	int m_nPort;
	bool m_bKeepLive;
	int m_nContentType;
	std::string m_strContent;
	std::string m_strUrl;
	std::string m_strUserAgent;
	std::string m_strHost;
	int m_nRequestType;
	int m_nTimeOut;
};

class HttpResponse
{
public:
	HttpResponse();
	~HttpResponse();
public:
	//获取状态码，200，400，404 etc.
	int GetStatusCode(void) const;

	//获取conetnettype，如：text/json，application/x-www-form-urlencoded，text/xml
	void GetContentType(std::string& strContentType) const;

	//设置连接是否保持，bKeepAlive=1保持:Keep-Alive，bKeepAlive=0不保持:Close
	bool GetKeepAlive() const;

	//获取回应包的包体
	size_t GetResponseBody(std::string& strRspBody) const;

public:
	//.......................................................
	//!!!这四个函数上层代码禁止调用!!!

	int		ParseHttpRequest(const char* data, size_t len, bool& over);

	//重置解析器
	void	ResetParser();

	//保存已接收到的数据
	void	SaveIncompleteData(const char* pData, size_t nLen);

	//重置一些控制标志
	void	ResetSomeCtrlFlag();

	//保存下来当前解析的字节数
	void	IncCurrTotalParseSize(size_t nSize);

	//获取当前已解析的字节数
	size_t	GetCurrTotalParseSize() const;

	//重置当前已解析的字节数
	void	ResetCurrTotalParseSize();

	//获取指针
	void*	GetParserPtr();

	void GetContentTypeStr(int nContentType, std::string& strContentType) const;
	//.......................................................
private:
	void* pParser;
	size_t m_nCurrTotalParseSize;

	bool m_bIncomplete;
	std::string m_strRequest;
	int m_nDefErrCode; //默认错误代码
};

/**************************************************************************************
* Function      : pfunc_on_msg_httpclient
* Description   : 回调函数：与回应相关的事件回调(消息回应、超时)
* Input         : [IN] void* pInvoker						上层调用的类指针
* Input         : [IN] uint64_t req_uuid					上层调用会话请求ID
* Input         : [IN] const boost::any& params				上层调用传入参数
* Input         : [IN] const HttpResponse& httpResponse		HttpResponse响应类
* Output        :
* Return        :
* Author        : Zhengbs     Version : 1.0    Date: 2018年01月15日
* Modify        : 无
* Others        : 无
**************************************************************************************/
typedef int(*pfunc_on_msg_httpclient)(void* pInvoker, uint64_t req_uuid, const boost::any& params, const HttpResponse& httpResponse);


//从网络层读取到信息的回调(适用于tcpclient)
//返回值0:代表成功 >0代表其他各种错误或反控等信息(待定) <0:底层会主动强制关闭连接
//nReadedLen用于控制残包粘包，如果是残包:必须填0，如果是粘包:必须填完整包的大小*完整包的个数
typedef int(*pfunc_on_readmsg_tcpclient)(void* pInvoker, uint64_t conn_uuid, const boost::any& param, const char* szMsg, int nMsgLen, int& nReadedLen);

typedef int(*pfunc_on_connection)(void* pInvoker, uint64_t conn_uuid, const boost::any& param, bool bConnected, const char* szIpPort);

typedef int(*pfunc_on_sendok)(void* pInvoker, const uint64_t conn_uuid, const boost::any& param);

typedef int(*pfunc_on_highwatermark)(void* pInvoker, const uint64_t conn_uuid, size_t highWaterMark);

typedef int(*pfunc_on_heartbeat)(uint64_t conn_uuid);

class tag_cli_mgr;
class HttpClientManager;
typedef std::shared_ptr<HttpClientManager> HttpClientManagerPtr;
class NetClient
	: public std::enable_shared_from_this<NetClient>
{
public:
	NetClient();
	~NetClient();

	// 可以使用单例，也可以不使用，如不使用直接net NetClient();
	static NetClient& Instance()
	{
		static auto instance_spr = std::make_shared<NetClient>();
		return *instance_spr.get();
	}

public:
	bool InitHttpClient(void* pInvoker,			/*上层调用的类指针*/
						int nThreadNum,			/*线程数*/
						int idleconntime,		/*空连接踢空时间*/
						int nMaxHttpClient,		/*最大并发数*/
						pfunc_on_msg_httpclient pFnOnMessage 	/*与回应相关的事件回调(recv)*/,
						size_t nDefRecvBuf,		//底层接收缓冲区常用的大小
						size_t nMaxRecvBuf,		//底层接收缓冲区最大的大小
						size_t nMaxSendQue);	//底层发送数缓冲最大的数据包个数
	int  SendHttpRequest(uint64_t req_uuid, const boost::any& params, const HttpRequest& httpRequest);
	void ExitHttpClient();

public:
	bool InitTcpClient(void* pInvoker,			/*上层调用的类指针*/
		int nThreadNum,			/*线程数*/
		int idleconntime,		/*空连接踢空时间*/
		int nMaxTcpClient,		/*最大并发数*/
		std::string ip,
		uint16_t  port,
		pfunc_on_connection pFnOnConnect,
		pfunc_on_sendok pFnOnSendOk,
		pfunc_on_readmsg_tcpclient pFnOnMessage, 	/*与回应相关的事件回调(recv)*/
		size_t nDefRecvBuf,// = 4 * 1024,		/*默认接收buf*/
		size_t nMaxRecvBuf,// = 4 * 1024,		/*最大接收buf*/
		size_t nMaxSendQue,// = 512			/*最大发送队列*/
		uint64_t& cli_uuid, /*返回的uuid*/
		size_t nMaxWnd /*最大窗口大小*/
	);
	int  SendTcpRequest(uint64_t cli_uuid, uint64_t cnn_uuid/*cnn_uuid = 0时， 系统自动选一个连接发送*/, 
		const boost::any& params, const char* szMsg, size_t nMsgLen);
	void ExitTcpClient(uint64_t cli_uuid);
	void SetHeartbeat(pfunc_on_heartbeat pFnOnHeartbeat, double interval = 10.0);

protected:
	bool InitTcpClient_bak(void* pInvoker,			/*上层调用的类指针*/
		int nThreadNum,			/*线程数*/
		int idleconntime,		/*空连接踢空时间*/
		int nMaxTcpClient,		/*最大并发数*/
		std::string ip,
		uint16_t  port,
		pfunc_on_connection pFnOnConnect,
		pfunc_on_sendok pFnOnSendOk,
		pfunc_on_readmsg_tcpclient pFnOnMessage, 	/*与回应相关的事件回调(recv)*/
		size_t nDefRecvBuf,// = 4 * 1024,		/*默认接收buf*/
		size_t nMaxRecvBuf,// = 4 * 1024,		/*最大接收buf*/
		size_t nMaxSendQue,// = 512			/*最大发送队列*/
		uint64_t& cli_uuid, /*返回的uuid*/
		size_t nMaxWnd /*最大窗口大小*/
	);
	int  SendTcpRequest_bak(uint64_t cli_uuid, uint64_t cnn_uuid/*cnn_uuid = 0时， 系统自动选一个连接发送*/,
		const boost::any& params, const char* szMsg, size_t nMsgLen);
	void ExitTcpClient_bak(uint64_t cli_uuid);


public:
	std::shared_ptr<tag_cli_mgr> m_climgr;
	std::mutex m_cli_lock;
private:
	HttpClientManagerPtr m_httpClientPtr;
	boost::any m_timerid_heartbeat;
	
};


//回调接口
typedef void(*CallBackRecv)(const void* /* pInvokePtr */, \
	int /* nLinkId */, \
	int /* command */, \
	int /* ret */, \
	const char* /* data */, \
	size_t /* len */, \
	int& /* nReadedLen */);
//参数
struct Params
{
	Params()
		: nWnd(512)
		, nDefBuf(4 * 1024)
		, nMaxBuf(4 * 1024 * 1024)
		, nMaxQue(512)
		, bAutoRecon(true)
		, nAutoReconTime(10)
		, nThreads(1)
		, bMayUseBlockOpration(true)
		, nClients(1)
	{
	}
	int nWnd;//滑动窗口
	int nDefBuf;	// 默认缓冲
	int nMaxBuf;	//最大缓冲
	int nMaxQue;	// 最大发送队列
	bool bAutoRecon;//是否自动重连
	int nAutoReconTime;//自动重连间隔(s)
	int nThreads;//线程个数
	bool bMayUseBlockOpration;	// 是否会用到阻塞操作
	int nClients;	// 客户端个数
};

class MwCli;
class XTcpClient
{
public:
	XTcpClient();
	~XTcpClient() {};

	static XTcpClient& Instance()
	{
		static XTcpClient instance;
		return instance;
	}
public:
	//初始化接口
	int Init(const void* pInvokePtr, const Params& params, CallBackRecv cb);
	//反初始化接口
	int UnInit();
	//运行时设置参数
	void SetRunParam(const Params& params);
	//申请链接, > 0 nLinkId, 其他失败, moder（0异步,1同步）, 
	int ApplyNewLink(const std::string ip, int port, int moder, int ms_timeout);
	//关闭链接
	int CloseLink(int nLinkId);
	//重新链接
	int ReConnLink(int nLinkId);
	//链接状态；这个接口不太靠谱，需要自己根据CallBackRecv判断
	bool LinkState(int nLinkId);
	//获取某个链接的当前缓冲大小
	size_t GetWaitBufSize(int nLinkId);
	//获取所有缓冲大小
	size_t GetAllWaitBufSize();
	//获取当前已使用多少窗口
	size_t GetUseWnd(int nLinkId);
	//发送接口
	int Send(int nLinkId, const char* data, size_t len);

private:
	const void* m_pInvokePtr;
	const Params m_params;
	std::shared_ptr<MwCli> m_mwcli_spr;
};


}
}

#endif