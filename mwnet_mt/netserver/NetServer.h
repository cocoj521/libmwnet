#ifndef MWNET_MT_NETSERVER_H
#define MWNET_MT_NETSERVER_H

#include <stdint.h>
#include <stdio.h>
#include <boost/any.hpp>
#include <string>
#include <memory>

namespace MWNET_MT
{
namespace SERVER
{
//content-type
enum HTTP_CONTENT_TYPE {CONTENT_TYPE_XML=1, CONTENT_TYPE_JSON, CONTENT_TYPE_URLENCODE, CONTENT_TYPE_UNKNOWN};
//POST/GET ...
enum HTTP_REQUEST_TYPE {HTTP_REQUEST_POST=1, HTTP_REQUEST_GET, HTTP_REQUEST_UNKNOWN};

class NetServerCtrlInfo;

typedef std::shared_ptr<NetServerCtrlInfo> NetServerCtrlInfoPtr;
class HttpRequest
{
public:
	HttpRequest(const std::string& strReqIp, uint16_t uReqPort);
	~HttpRequest();
public:
	//是否需要保持连接,即connection是keep-alive还是close
	//返回值:true:keep-alive false:close
	bool	IsKeepAlive() const;

	//返回整个http请求的报文长度(报文头+报文体)
	size_t  GetHttpRequestTotalLen() const;

	//获取http body的数据
	//返回值:body的内容和长度
	size_t	GetBodyData(std::string& strBody) const;

	//获取请求url
	//返回值:url的内容以及长度
	//说明:如:post请求,POST /sms/std/single_send HTTP/1.1 ,/sms/std/single_send就是URL
	//	   如:get请求,
	size_t	GetUrl(std::string& strUrl) const;

	//获取请求客户端的IP
	//返回值:请求客户端的IP
	std::string GetClientRequestIp() const;

	//获取请求客户端的端口
	//返回值:请求客户端的端口
	uint16_t GetClientRequestPort() const;

	//获取User-Agent
	//返回值:User-Agent的内容以及长度
	size_t	GetUserAgent(std::string& strUserAgent) const;

	//获取content-type
	//返回值:CONTENT_TYPE_XML 代表 xml,CONTENT_TYPE_JSON 代表 json,CONTENT_TYPE_URLENCODE 代表 urlencode,CONTENT_TYPE_UNKNOWN 代表 未知类型
	//说明:暂只提供这三种
	int		GetContentType() const;
	std::string GetContentTypeStr();

	//获取host
	//返回值:host的内容以及长度
	size_t	GetHost(std::string& strHost) const;

	//获取x-forwarded-for
	//返回值:X-Forwarded-For的内容以及长度
	size_t	GetXforwardedFor(std::string& strXforwardedFor) const;

	//获取http请求的类型
	//返回值:HTTP_REQUEST_POST 代表 post请求; HTTP_REQUEST_GET 代表 get请求 ; HTTP_REQUEST_UNKNOWN 代表 未知(解析包头有错误)
	int		GetRequestType() const;

	//获取SOAPAction(仅当是soap时才有用)
	//返回值:SOAPAction内容以及长度
	size_t	GetSoapAction(std::string& strSoapAction) const;

	//获取Wap Via
	//返回值:via内容以及长度
	size_t	GetVia(std::string& strVia) const;

	//获取x-wap-profile
	//返回值:x-wap-profile内容以及长度
	size_t	GetXWapProfile(std::string& strXWapProfile) const;

	//获取x-browser-type
	//返回值:x-browser-type内容以及长度
	size_t	GetXBrowserType(std::string& strXBrowserType) const;

	//获取x-up-bear-type
	//返回值:x-up-bear-type内容以及长度
	size_t	GetXUpBearType(std::string& strXUpBearType) const;

	//获取整个http报文的内容(头+体)
	//返回值:http报文的指针,以及长度
	size_t	GetHttpRequestContent(std::string& strHttpRequestContent) const;

	//获取http头中任一个域对应的值(域不区分大小写)
	//返回值:域对应的值
	size_t	GetHttpHeaderFieldValue(const char* field, std::string& strValue) const;
	
	//获取唯一的http请求ID
	//返回值:请求ID
	uint64_t	GetHttpRequestId() const;

	//获取收到完整http请求的时间
	//返回值:请求时间 
	uint64_t	GetHttpRequestTime() const;

//..........................附带提供一组解析urlencode的函数......................
	//解析key,value格式的body,如:cmd=aa&seqid=1&msg=11
	//注意:仅当GetContentType返回CONTENT_TYPE_URLENCODE才可以使用该函数去解析
	bool	ParseUrlencodedBody(const char* pbody/*http body 如:cmd=aa&seqid=1&msg=11*/, size_t len/*http body的长度*/) const;

	//通过key获取value的指针和长度,不做具体拷贝.若返回false代表key不存在
	//注意:必须先调用ParseUrlencodedBody后才可以使用该函数
	bool	GetUrlencodedBodyValue(const char* key/*key,如:cmd=aa,cmd就是key,aa就是value*/, std::string& value) const;
//................................................................................
public:
//.......................................................
//!!!以下函数上层代码禁止调用!!!

	int		ParseHttpRequest(const char* data, size_t len, size_t maxlen, bool& over, const uint64_t& request_id, const int64_t& request_time);

	//重置一些控制标志
	void	ResetSomeCtrlFlag();

	//置已经回复过100-continue标志
	void 	SetHasRsp100Continue();

	//是否需要回复100continue;
	bool 	HasRsp100Continue();

	//报文头中是否有100-continue;
	bool	b100ContinueInHeader();

	//重置解析器
	void	ResetParser();

	//保存已接收到的数据
	void	SaveIncompleteData(const char* pData, size_t nLen);
//.......................................................
private:
	void* p;
	std::string m_strReqIp;
	uint16_t m_uReqPort;
	bool m_bHasRsp100Continue;
	bool m_bIncomplete;
	std::string m_strRequest;
	uint64_t m_request_id;
	int64_t m_request_tm;
};

class HttpResponse
{
public:
	HttpResponse();
	~HttpResponse();
public:
	//设置状态码，200，400，404 etc.
	//注意:该函数内部有校验，只允许调用一次
	void SetStatusCode(int nStatusCode);

	//设置conetnettype，如：text/json，application/x-www-form-urlencoded，text/xml
	//注意:该函数内部有校验，只允许调用一次
	void SetContentType(const std::string& strContentType);

    //设置Location，如：当状态码为301、302进行协议跳转时，需要设置该参数指定跳转地址
	//注意:该函数内部有校验，只允许调用一次
	void SetLocation(const std::string& strLocation);

	//设置一些没有明确定义的非标准的http回应头
	//格式如:x-token: AAA\r\nx-auth: BBB\r\n
	//注意:必须加\r\n做为每一个域的结束
	void SetHeader(const std::string& strFieldAndValue);
	
	//设置连接是否保持，bKeepAlive=1保持:Keep-Alive，bKeepAlive=0不保持:Close
	//注意:该函数内部有校验，只允许调用一次
	void SetKeepAlive(bool bKeepAlive);

	//是否需要保持长连接
	bool IsKeepAlive() const;

	//设置回应包的包体
	//注意:必须在SetStatusCode&SetContentType&SetKeepAlive都设置OK了以后才可以调用
	//注意:该函数内部有校验，只允许调用一次
	void SetResponseBody(const std::string& strRspBody);

	//将上层代码格式化好的完整的回应报文（头+体）设置进来
	//使用该函数时SetStatusCode&SetContentType&SetKeepAlive仍可以同时设置，但SetResponseBody一定不能调用
	void SetHttpResponse(std::string& strResponse);

	//将与该response对应的http请求的requestId设置进来,request_id通过HttpRequest中GetHttpRequestId获取
	void SetHttpRequestId(const uint64_t& request_id);

	//将与该response对应的http请求的requestTime设置进来,request_time通过HttpRequest中GetHttpRequestTime获取
	void SetHttpRequestTime(const int64_t& request_time);

	//返回与该response对应的http请求的requestId
	uint64_t GetHttpRequestId() const;

	//返回与该response对应的http请求的requestTime
	int64_t GetHttpRequestTime() const;

	//返回格式化好的httpresponse字符串
	//注意:必须在调用SetResponseBody或SetHttpResponse以后才可以调用
	const std::string& GetHttpResponse() const;

	//返回http回应的总长度(头+体)
	//注意:必须在SetResponseBody设置OK了以后才可以调用
	size_t GetHttpResponseTotalLen() const;
private:
	void FormatHttpResponse(const std::string& strRspBody);
private:
	std::string m_strResponse;
	std::string m_strContentType;
    std::string m_strLocation;
	std::string m_strOtherFieldValue;
	int  m_nStatusCode;
	bool m_bKeepAlive;
	bool m_bHaveSetStatusCode;
	bool m_bHaveSetContentType;
	bool m_bHaveSetLocation;
	bool m_bHaveSetKeepAlive;
	bool m_bHaveSetResponseBody;
	uint64_t m_request_id;
	int64_t m_request_tm;
};

//////////////////////////////////////////////////////回调函数//////////////////////////////////////////////////////////////
//上层调用代码在实现时，必须要快速操作，不可以加锁，否则将阻塞底层处理，尤其要注意，不能加锁，否则底层多线程将变成单线程

	//与连接相关的回调(连接/关闭等)
	//返回值0:代表成功 >0代表其他各种错误或反控等信息(待定) <0:底层会主动强制关闭连接
	//sessioninfo为调用settcpsessioninfo,sethttpsessioninfo时传入的sessioninfo
	typedef int (*pfunc_on_connection)(void* pInvoker, uint64_t conn_uuid, const boost::any& conn, const boost::any& sessioninfo, bool bConnected, const char* szIpPort);

	//从网络层读取到信息的回调(适用于tcpserver)
	//返回值0:代表成功 >0代表其他各种错误或反控等信息(待定) <0:底层会主动强制关闭连接
	//nReadedLen用于控制残包粘包，如果是残包:必须填0，如果是粘包:必须填完整包的大小*完整包的个数
	//sessioninfo为调用settcpsessioninfo,sethttpsessioninfo时传入的sessioninfo
	typedef int (*pfunc_on_readmsg_tcp)(void* pInvoker, uint64_t conn_uuid, const boost::any& conn, const boost::any& sessioninfo, const char* szMsg, int nMsgLen, int& nReadedLen);

	//从网络层读取到信息的回调(适用于httpserver)
	//返回值0:代表成功 >0代表其他各种错误或反控等信息(待定) <0:底层会主动强制关闭连接
	//注意:pHttpReq离开回调函数后指针将自失效,绝对不可以仅保存指针至后续业务代码中使用,必须在离开回调函数前保存有用数据!!!
	//sessioninfo为调用settcpsessioninfo,sethttpsessioninfo时传入的sessioninfo
	typedef int (*pfunc_on_readmsg_http)(void* pInvoker, uint64_t conn_uuid, const boost::any& conn, const boost::any& sessioninfo, const HttpRequest* pHttpReq);

	//发送数据完成的回调
	//返回值0:代表成功 >0代表其他各种错误或反控等信息(待定) <0:底层会主动强制关闭连接
	//params为调用sendhttpmsg,sendtcpmsg时传入的params
	//sessioninfo为调用settcpsessioninfo,sethttpsessioninfo时传入的sessioninfo
	typedef int (*pfunc_on_sendok)(void* pInvoker, const uint64_t conn_uuid, const boost::any& conn, const boost::any& sessioninfo, const boost::any& params);

	// 发送数据失败的回调,参数意义与pfunc_on_sendok相同
	// errCode:
	//1:连接已不可用        
	//2:发送缓冲满(暂时不会返回,暂时通过高水位回调告知上层)       
	//3:调用write函数失败 
	//4:发送超时(未在设置的超时间内发送成功)
	typedef int(*pfunc_on_senderr)(void* pInvoker, const uint64_t conn_uuid, const boost::any& conn, const boost::any& sessioninfo, const boost::any& params, int errCode);

	//高水位回调 当底层缓存的待发包数量超过设置的值时会触发该回调
	//返回值0:代表成功 >0代表其他各种错误或反控等信息(待定) <0:底层会主动强制关闭连接
	typedef int (*pfunc_on_highwatermark)(void* pInvoker, const uint64_t conn_uuid, const boost::any& conn, const boost::any& sessioninfo, size_t highWaterMark);
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

//关于是否开启端口复用:
//短连接较多的情况建议启用端口复用,这样可以提高短连接建连能力
//长连接较多的不建议启用,因为启用后连接分配由系统完成,无法确保每个IO线程上连接的平均性

class NetServer
	{
	static const size_t kDefRecvBuf_httpsvr = 4  * 1024;
	static const size_t kMaxRecvBuf_httpsvr = 32 * 1024;
	static const size_t kMaxSendQue_httpsvr = 16;
	static const size_t kDefRecvBuf_tcpsvr  = 4  * 1024;
	static const size_t kMaxRecvBuf_tcpsvr  = 32 * 1024;
	static const size_t kMaxSendQue_tcpsvr  = 64;

	public:
		//初始化并启动server
		//对于有生成唯一连接ID的场景可使用本构造函数
		NetServer(uint16_t unique_node_id/*系统级唯一的网结层结点ID,1~8000间取值*/);
		NetServer();
		//设置nodeid(1~8000间取值)
		void SetUniqueNodeId(uint16_t unique_node_id);
		~NetServer();
	public:
		// 开启或关闭接收/发送日志,默认日志全部是自动开启的,若要一启动就自动关闭日志,需要在startserver调用以下函数关闭
		void 	EnableHttpServerRecvLog(bool bEnable=true);
		void 	EnableHttpServerSendLog(bool bEnable=true);
		void 	EnableTcpServerRecvLog(bool bEnable=true);
		void 	EnableTcpServerSendLog(bool bEnable=true);
	public:
		//启动TCP服务
		void 	StartTcpServer(void* pInvoker,		/*上层调用的类指针*/
								uint16_t listenport,	/*监听端口*/
								int threadnum,			/*线程数*/
								int idleconntime,		/*空连接踢空时间*/
								int maxconnnum,			/*最大连接数*/
								bool bReusePort,		/*是否启用SO_REUSEPORT*/
								pfunc_on_connection pOnConnection, 		/*与连接相关的事件回调(connect/close/error)*/
								pfunc_on_readmsg_tcp pOnReadMsg, 	   	/*网络层收到数据的事件回调*/
								pfunc_on_sendok pOnSendOk,				/*发送数据完成的事件回调*/
								pfunc_on_senderr pOnSendErr,			/*发送数据失败的事件回调*/
								pfunc_on_highwatermark pOnHighWaterMark=NULL,/*高水位事件回调*/
								size_t nDefRecvBuf = kDefRecvBuf_tcpsvr,	/*每个连接的默认接收缓冲,与nMaxRecvBuf一样根据真实业务数据包大小进行设置,该值设置为大部分业务包可能的大小*/
								size_t nMaxRecvBuf = kMaxRecvBuf_tcpsvr,   	/*每个连接的最大接收缓冲,若超过最大接收缓冲仍然无法组成一个完整的业务数据包,连接将被强制关闭,取值时务必结合业务数据包大小进行填写*/
								size_t nMaxSendQue = kMaxSendQue_tcpsvr		/*每个连接的最大发送缓冲队列,网络层最多可以容忍的未发送出去的包的总数,若超过该值,将会返回高水位回调,上层必须根据pfunc_on_sendok控制好滑动*/
								);

		//停止TCP服务
		void 	StopTcpServer();

		//发送TCP信息 
		//conn_uuid:连接建立时返回的连接惟一ID
		//conn:连接建立时返回的连接对象
		//params:自定义发送参数，发送完成/失败回调时会返回，请用智能指针
		//szMsg,nMsgLen:要发送数据的内容和长度
		//timeout:要求底层多长时间内必须发出，若未发送成功返回失败的回调.单位:毫秒,默认为0,代表不要求超时间
		//bKeepAlive:发完以后是否保持连接.若设置为false，当OnSendOk回调完成后，会自动断开连接
		//0:成功    1:当前连接已不可用          2:当前连接最大发送缓冲已满
		int 	SendMsgWithTcpServer(uint64_t conn_uuid, const boost::any& conn, const boost::any& params, const char* szMsg, size_t nMsgLen, int timeout=0, bool bKeepAlive=true);

		//暂停当前TCP连接的数据接收
		//conn_uuid:连接建立时返回的连接惟一ID
		//conn:连接建立时返回的连接对象
		//delay:暂停接收的时长.单位:毫秒,默认为10ms,<=0时不延时
		void 	SuspendTcpConnRecv(uint64_t conn_uuid, const boost::any& conn, int delay);

		//恢复当前TCP连接的数据接收
		//conn_uuid:连接建立时返回的连接惟一ID
		//conn:连接建立时返回的连接对象
		void 	ResumeTcpConnRecv(uint64_t conn_uuid, const boost::any& conn);

		//强制关闭TCP连接
		void 	ForceCloseTcpConnection(uint64_t conn_uuid, const boost::any& conn);

		//获取当前TCP连接总数
		int 	GetCurrTcpConnNum();

		//获取当前TCP总请求数
		int64_t GetTotalTcpReqNum();

		//获取当前TCP总的等待发送的数据包数量
		int64_t GetTotalTcpWaitSendNum();

		//重置TCP空连接踢空时间
		void	ResetTcpIdleConnTime(int idleconntime);

		//重置TCP可支持的最大连接数
		void	ResetTcpMaxConnNum(int maxconnnum);
		
		//设置tcp会话的信息(请传session类的智能指针，并且只允许调用一次&session类中自行控制多线程互斥问题)
		void	SetTcpSessionInfo(uint64_t conn_uuid, const boost::any& conn, const boost::any& sessioninfo);

		//设置高水位值 (thread safe)
		void 	SetTcpHighWaterMark(uint64_t conn_uuid, const boost::any& conn, size_t highWaterMark);
	public:
		//启动HTTP服务
		void StartHttpServer(void* pInvoker,		/*上层调用的类指针*/
								uint16_t listenport,	/*监听端口*/
								int threadnum,			/*线程数*/
								int idleconntime,		/*空连接踢空时间*/
								int maxconnnum,			/*最大连接数*/
								bool bReusePort,		/*是否启用SO_REUSEPORT*/
								pfunc_on_connection pOnConnection, 		/*与连接相关的事件回调(connect/close/error)*/
								pfunc_on_readmsg_http pOnReadMsg, 	   	/*网络层收到数据的事件回调*/
								pfunc_on_sendok pOnSendOk,				/*发送数据完成的事件回调*/
								pfunc_on_senderr pOnSendErr,			/*发送数据失败的事件回调*/
								pfunc_on_highwatermark pOnHighWaterMark=NULL,/*高水位事件回调*/
								size_t nDefRecvBuf = kDefRecvBuf_httpsvr,	/*每个连接的默认接收缓冲,与nMaxRecvBuf一样根据真实业务数据包大小进行设置,该值设置为大部分业务包可能的大小*/
								size_t nMaxRecvBuf = kMaxRecvBuf_httpsvr,	/*每个连接的最大接收缓冲,若超过最大接收缓冲仍然无法组成一个完整的业务数据包,连接将被强制关闭,取值时务必结合业务数据包大小进行填写*/
								size_t nMaxSendQue = kMaxSendQue_httpsvr 	/*每个连接的最大发送缓冲队列,网络层最多可以容忍的未发送出去的包的总数,若超过该值,将会返回高水位回调,上层必须根据pfunc_on_sendok控制好滑动*/
								);

		//停止HTTP服务
		void 	StopHttpServer();

		//发送HTTP回应
		//conn_uuid:连接建立时返回的连接惟一ID
		//conn:连接建立时返回的连接对象
		//params:自定义发送参数，发送完成/失败回调时会返回，请用智能指针
		//rsp:要发送response
		//timeout:要求底层多长时间内必须发出，若未发送成功返回失败的回调.单位:毫秒,默认为0,代表不要求超时间
		//0:成功    1:当前连接已不可用          2:当前连接最大发送缓冲已满
		int 	SendHttpResponse(uint64_t conn_uuid, const boost::any& conn, const boost::any& params, const HttpResponse& rsp, int timeout=0);

		//强制关闭HTTP连接
		void 	ForceCloseHttpConnection(uint64_t conn_uuid, const boost::any& conn);

		//获取当前HTTP连接总数
		int 	GetCurrHttpConnNum();

		//获取当前HTTP总请求数
		int64_t GetTotalHttpReqNum();

		//获取当前HTTP总的等待发送的数据包数量
		int64_t GetTotalHttpWaitSendNum();

		//重置HTTP空连接踢空时间
		void	ResetHttpIdleConnTime(int idleconntime);

		//重置HTTP可支持的最大连接数
		void	ResetHttpMaxConnNum(int maxconnnum);

		//设置http会话的信息(请传session类的智能指针，并且只允许调用一次&session类中自行控制多线程互斥问题)
		void	SetHttpSessionInfo(uint64_t conn_uuid, const boost::any& conn, const boost::any& sessioninfo);

		//设置高水位值 (thread safe)
		void	SetHttpHighWaterMark(uint64_t conn_uuid, const boost::any& conn, size_t highWaterMark);

	private:

		// 全局控制变量
		NetServerCtrlInfoPtr m_NetServerCtrl;
	};
}
}
#endif  // MWNET_MT_NETSERVER_H

