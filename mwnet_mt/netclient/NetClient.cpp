#include "NetClient.h"
#include "httpclient.h"
#include "TcpClient.h"

#include <mwnet_mt/net/httpparser/httpparser.h>
#include <mwnet_mt/net/TimerId.h>
#include <iostream>
#include <memory>
#include <thread>
#include <chrono>
#include <atomic>

using namespace mw_http_parser;


using namespace mwnet_mt;
using namespace mwnet_mt::net;
#include "../net/EventLoopThread.h"
#include "../net/EventLoop.h"

namespace MWNET_MT{
namespace CLIENT{

size_t g_nMaxRecvBuf_Http;

HttpRequest::HttpRequest() { }

void HttpRequest::GetContentTypeStr(int nContentType, std::string& strContentType) const
{
	switch (nContentType)
	{
	case CONTENT_TYPE_URLENCODE:
		strContentType = "application/x-www-form-urlencoded";
		break;
	case CONTENT_TYPE_XML:
		strContentType = "text/xml";
		break;
	case CONTENT_TYPE_JSON:
		strContentType = "text/json";
		break;
	default:
		strContentType = "unknow";
		break;
	}
}

HttpRequest::HttpRequest(const std::string& strReqIp, int nReqPort) :
m_strIp("127.0.0.1"),
m_nPort(80),
m_bKeepLive(false),
m_nContentType(CONTENT_TYPE_URLENCODE),
m_strContent(""),
m_strUrl(""),
m_strUserAgent("Mozilla/5.0 (Windows; U; Windows NT 5.1; zh-CN; rv:1.9.2.3) Gecko/20100401 Firefox/3.6.3"),
m_strHost("127.0.0.1"),
m_nRequestType(HTTP_REQUEST_POST),
m_nTimeOut(3)
{
	m_strIp = strReqIp;
	m_nPort = nReqPort;
}

HttpRequest::~HttpRequest()
{

}

void HttpRequest::SetKeepAlive(bool bKeepLive)
{ 
	m_bKeepLive = bKeepLive; 
}

bool HttpRequest::GetKeepAlive() const
{ 
	return m_bKeepLive; 
}

void HttpRequest::SetContent(const std::string& strContent)
{ 
	m_strContent = strContent; 
}

void HttpRequest::SetUrl(const std::string& strUrl)
{ 
	m_strUrl = strUrl; 
}

void HttpRequest::SetRequestIp(const std::string& strIp)
{ 
	m_strIp = strIp; 
}

void HttpRequest::SetRequestPort(int nPort)
{ 
	m_nPort = nPort; 
}

void HttpRequest::SetUserAgent(const std::string& strUserAgent)
{ 
	m_strUserAgent = strUserAgent; 
}

void HttpRequest::SetContentType(int nContentType)
{ 
	m_nContentType = nContentType; 
}

void HttpRequest::SetHost(const std::string& strHost)
{ 
	m_strHost = strHost; 
}

void HttpRequest::SetRequestType(int nRequestType)
{ 
	m_nRequestType = nRequestType; 
}

int HttpRequest::GetTimeOut()const
{ 
	return m_nTimeOut; 
}

//���ó�ʱ
void HttpRequest::SetTimeOut(int nTimeOut)
{ 
	m_nTimeOut = nTimeOut; 
}

void HttpRequest::GetRequestIp(std::string& strIp) const
{ 
	strIp = m_strIp; 
};

int	HttpRequest::GetRequestPort(void) const
{ 
	return m_nPort; 
};

void HttpRequest::GetRequestStr(std::string& strRequestStr) const 
{
	std::stringstream streamBuff;

	//����ʽ
	if (HTTP_REQUEST_POST == m_nRequestType)
	{
		streamBuff << "POST " << m_strUrl;
		streamBuff << " HTTP/1.1" << ENTER;
		streamBuff << "Host: " << m_strHost << ENTER;
		streamBuff << "User-Agent: " << m_strUserAgent << ENTER;

		std::string strContentType = "";
		GetContentTypeStr(m_nContentType, strContentType);

		streamBuff << "Content-Type:" << strContentType << ENTER;
		streamBuff << "Content-Length:" << m_strContent.length() << ENTER;
		streamBuff << "Connection:" << (m_bKeepLive ? "Keep-alive" : "Close") << ENTER << ENTER;

		streamBuff << m_strContent.c_str();
	}
	else if (HTTP_REQUEST_GET == m_nRequestType)
	{
		streamBuff << "GET " << m_strUrl << "?" << m_strContent;
		streamBuff << " HTTP/1.1" << ENTER;
		streamBuff << "Host: " << m_strHost << ENTER;
		streamBuff << "User-Agent: " << m_strUserAgent << ENTER;
		streamBuff << "Connection:" << (m_bKeepLive ? "Keep-alive" : "Close") << ENTER << ENTER;
	}

	strRequestStr = streamBuff.str();
}

HttpResponse::HttpResponse()
{
	pParser = reinterpret_cast<void*>(new HttpParser(HTTP_RESPONSE));
	m_nCurrTotalParseSize = 0;
	m_bIncomplete = false;
	m_nDefErrCode = 0;
}

HttpResponse::~HttpResponse()
{
	if (pParser)
	{
		delete reinterpret_cast<HttpParser*>(pParser);
		pParser = nullptr;
	}

}

//���ý�����
void HttpResponse::ResetParser()
{
	HttpParser* psr = reinterpret_cast<HttpParser*>(pParser);
	if (psr)
	{
		psr->reset_parser();
	}
}

//�����ѽ��յ�������
void HttpResponse::SaveIncompleteData(const char* pData, size_t nLen)
{
	m_strRequest.append(pData, nLen);
}

//����һЩ���Ʊ�־
void HttpResponse::ResetSomeCtrlFlag()
{
	m_bIncomplete = false;
	m_strRequest = "";
	m_nDefErrCode = 0;
}

// ����http����
// ����ֵ 0:�ɹ� ��ʱҪ��over�Ƿ�Ϊtrue,��Ϊtrue����������,��Ϊfalse�����δ��ȫ
// ����ֵ 1:�������ʧ�� ��ر�����
int	HttpResponse::ParseHttpRequest(const char* data, size_t len, bool& over)
{
	int nRet = 1;

	HttpParser* parser = reinterpret_cast<HttpParser*>(pParser);
	if (parser)
	{
		// �����δ�������İ�,�ȸ����ϵ��ϴα���Ĳ���,Ȼ����ʹ�ô洢�Ĳ��ֽ��н���			
		if (m_bIncomplete) SaveIncompleteData(data, len);
		nRet = parser->execute_http_parse(m_bIncomplete ? m_strRequest.c_str() : data, m_bIncomplete ? m_strRequest.size() : len, over);
		// ��û���꣬�����գ����ûص�, �����뱣������, �����ñ�־�ð���ʹ���ϴα������������ݽ����´ν���
		if (!m_bIncomplete && 0 == nRet && !over)
		{
			SaveIncompleteData(data, len);
			m_bIncomplete = true;
		}

		// �������g_nMaxRecvBuf_Http��������Ȼ����һ��������,��ʱ�ر�����
		if (!over && m_strRequest.size() >= g_nMaxRecvBuf_Http)
		{
			nRet = 1;
			m_nDefErrCode = -101;
		}
	}

	return nRet;
}

void HttpResponse::IncCurrTotalParseSize(size_t nSize)
{
	m_nCurrTotalParseSize += nSize;
}

size_t	HttpResponse::GetCurrTotalParseSize() const
{
	return m_nCurrTotalParseSize;
}

void HttpResponse::GetContentTypeStr(int nContentType, std::string& strContentType) const
{
	switch (nContentType)
	{
	case CONTENT_TYPE_URLENCODE:
		strContentType = "application/x-www-form-urlencoded";
		break;
	case CONTENT_TYPE_XML:
		strContentType = "text/xml";
		break;
	case CONTENT_TYPE_JSON:
		strContentType = "text/json";
		break;
	default:
		strContentType = "unknow";
		break;
	}
}


void HttpResponse::ResetCurrTotalParseSize()
{
	m_nCurrTotalParseSize = 0;
}

void* HttpResponse::GetParserPtr()
{
	return pParser;
}

int HttpResponse::GetStatusCode(void) const 
{
	if (0 == m_nDefErrCode)
	{
		int nCode = 408;
		HttpParser* parser = reinterpret_cast<HttpParser*>(pParser);

		if (parser)
		{
			nCode = parser->get_http_response_status_code();
			if (0 == nCode)
			{
				nCode = 408; // ����Ϊ��ʱ
			}
		}

		return nCode;
	}

	return m_nDefErrCode;
}

void HttpResponse::GetContentType(std::string& strContentType) const
{
	strContentType = "";

	HttpParser* parser = reinterpret_cast<HttpParser*>(pParser);

	if (parser)
	{
		int nContentType = parser->get_http_content_type();
		GetContentTypeStr(nContentType, strContentType);
	}
}

bool HttpResponse::GetKeepAlive() const
{
	bool bRet = false;

	HttpParser* parser = reinterpret_cast<HttpParser*>(pParser);

	if (parser)
	{
		bRet = parser->should_keep_alive();
	}

	return bRet;
}

// void HttpResponse::GetResponseBody(std::string& strRspBody) const
// {
// 	strRspBody = "";
// 
// 	HttpParser* parser = reinterpret_cast<HttpParser*>(pParser);
// 
// 	if (parser)
// 	{
// 		size_t nLen = 0;
// 
// 		char *pBuff = nullptr;
// 
// 		parser->get_http_body_data(&pBuff, &nLen);
// 
// 		if (pBuff)
// 		{
// 			strRspBody.append(pBuff, nLen);
// 		}
// 	}
// }

// ����ֵ:body�����ݺͳ���
size_t HttpResponse::GetResponseBody(std::string& strRspBody) const
{
	HttpParser* psr = reinterpret_cast<HttpParser*>(pParser);
	if (psr)
	{
		size_t len = psr->get_http_body_len();
		if (psr->is_chunk_body())
		{
			strRspBody.resize(len);
			psr->get_http_body_chunk_data(const_cast<char*>(strRspBody.data()));
		}
		else
		{
			char* data = NULL; len = 0;
			psr->get_http_body_data(&data, &len);
			strRspBody.assign(data, len);
		}
		return len;
	}
	return 0;
}



class xTcpClientSingle;
typedef std::shared_ptr<xTcpClientSingle> xTcpClientSinglePtr;
typedef std::shared_ptr<mwnet_mt::net::EventLoopThread> EventLoopThreadPtr;
typedef std::weak_ptr<TcpConnection> WeakTcpConnectionPtr;
class xTcpClientSingle
	: public std::enable_shared_from_this<xTcpClientSingle>
{
public:
	xTcpClientSingle(void* pInvoker, EventLoop* loop, const InetAddress& svrAddr,
		pfunc_on_connection pOnConnection,
		pfunc_on_readmsg_tcpclient pOnReadMsg,
		pfunc_on_sendok pOnSendOk,
		pfunc_on_highwatermark pOnHighWaterMark,
		size_t nDefRecvBuf, size_t nMaxRecvBuf, size_t nMaxSendQue)
		:
		m_pInvoker(pInvoker),
		loop_(loop),
		highWaterMark_(nMaxSendQue),
		maxRecvBuf_(nMaxRecvBuf),
		m_pfunc_on_connection(pOnConnection),
		m_pfunc_on_readmsg_tcp(pOnReadMsg),
		m_pfunc_on_sendok(pOnSendOk),
		m_pfunc_on_highwatermark(pOnHighWaterMark),
		conn_status_(false),
		client_(loop, svrAddr, "TcpClient", nDefRecvBuf, nMaxRecvBuf, nMaxSendQue)
	{
		client_.setConnectionCallback(std::bind(&xTcpClientSingle::onConnection, this, std::placeholders::_1));
		//client_.setConnectionCallback(xdef_onConnection);

		client_.setMessageCallback(std::bind(&xTcpClientSingle::onMessage, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
		client_.setWriteCompleteCallback(std::bind(&xTcpClientSingle::onWriteOk, this, std::placeholders::_1, std::placeholders::_2));
	}

	~xTcpClientSingle()
	{
	}

	void Connect(const xTcpClientSinglePtr& self)
	{
		self_ = self;
		client_.connect();
	}

	void DisConnect()
	{
		client_.disconnect();
	}

	void Stop()
	{
		client_.stop();
	}

	bool is_ok()
	{
		auto cnn = client_.connection();
		if (cnn)
		{
			return cnn->connected();
		}
		return false;
	}

	void SetHighWaterMark(size_t highwatermark)
	{
		highWaterMark_ = highwatermark;
	}

	int SendTcpRequest(const boost::any& params, const char* szMsg, size_t nMsgLen, bool bKeepAlive)
	{
		int nRet = -1;
		TcpConnectionPtr pConn(weak_conn_.lock());
		if (pConn)
		{
			if (!bKeepAlive) pConn->setNeedCloseFlag();
			pConn->send(szMsg, static_cast<int>(nMsgLen), params);
			nRet = 0;
		}
		return nRet;
	}

	int64_t getConnuuid()
	{
		TcpConnectionPtr pConn(weak_conn_.lock());
		if (pConn)
		{
			return pConn->getConnuuid();
		}
		return 0;
	}

	std::string getIpPort()
	{
		return client_.getInetAddress().toIpPort().c_str();
	}
private:
	void onConnection(const TcpConnectionPtr& conn)
	{
		if (!conn)
		{
			if (m_pfunc_on_connection)
			{
				m_pfunc_on_connection(m_pInvoker, 0, NULL, false,
					client_.getInetAddress().toIpPort().c_str());
			}

			if (self_) self_.reset();
			return;
		}

		if (conn->connected())
		{
			weak_conn_ = conn;
			conn_status_ = true;

			uint64_t conn_uuid = MakeConnuuid(1000);
			conn->setConnuuid(conn_uuid);

			// conn->setHighWaterMarkCallback(std::bind(&xTcpClientSingle::onHighWater, shared_from_this(), std::placeholders::_1, std::placeholders::_2), highWaterMark_);

			if (m_pfunc_on_connection)
			{
				int nRet = m_pfunc_on_connection(m_pInvoker, conn->getConnuuid(), NULL, true,
					conn->peerAddress().toIpPort().c_str());
				if (nRet < 0)
				{
					conn->shutdown();
					conn->forceClose();
				}
				else if (0 == nRet)
				{
				}
				else
				{
					//.......
				}
			}
		}
		else if (conn->disconnected())
		{
			conn_status_ = false;
			if (m_pfunc_on_connection)
			{
				m_pfunc_on_connection(m_pInvoker, conn->getConnuuid(), NULL, false,
					conn->peerAddress().toIpPort().c_str());
			}
			if (self_) self_.reset();
		}

	}

	void onMessage(const TcpConnectionPtr& conn, Buffer* buf, Timestamp time)
	{
		if (!conn) return;

		if (m_pfunc_on_readmsg_tcp)
		{
			int nReadedLen = 0;
			StringPiece RecvMsg = buf->toStringPiece();

			int nRet = m_pfunc_on_readmsg_tcp(m_pInvoker, conn->getConnuuid(), NULL, RecvMsg.data(), RecvMsg.size(), nReadedLen);
			if (nReadedLen == RecvMsg.size())
			{
				buf->retrieveAll();
			}
			else
			{
				buf->retrieve(nReadedLen);

				if (buf->readableBytes() >= maxRecvBuf_)
				{
					nRet = -1;
					// LOG_WARN<<"[TCP][RECV]["<<conn->getConnuuid()<<"]\nillegal data size:"<<buf->readableBytes()<<",maybe too long\n";
				}
			}
			if (nRet < 0)
			{
				conn->shutdown();
				conn->forceClose();
			}
			else if (0 == nRet)
			{
			}
			else
			{
				//.......
			}
		}
	}

	void onWriteOk(const TcpConnectionPtr& conn, const boost::any& params)
	{
		if (!conn) return;

		int nRet = 0;
		if (m_pfunc_on_sendok)
		{
			nRet = m_pfunc_on_sendok(m_pInvoker, conn->getConnuuid(), params);
		}

		if (nRet < 0 || conn->bNeedClose())
		{
			conn->shutdown();
			conn->forceClose();
		}
	}

	void onHighWater(const TcpConnectionPtr& conn, size_t highWaterMark)
	{
		if (!conn) return;

		// 		int nRet = 0;
		// 		if (m_pfunc_on_highwatermark)
		// 		{
		// 			nRet = m_pfunc_on_highwatermark(m_pInvoker, conn->getConnuuid(), highWaterMark);
		// 		}
		// 
		// 		if (nRet < 0 || conn->bNeedClose())
		// 		{
		// 			conn->shutdown();
		// 			//conn->forceCloseWithDelay(1.0);  // > round trip of the whole Internet.
		// 			conn->forceClose();
		// 		}
	}

private:
	void* m_pInvoker;
	EventLoop* loop_;
	size_t highWaterMark_;
	size_t maxRecvBuf_;
	pfunc_on_connection m_pfunc_on_connection;
	pfunc_on_readmsg_tcpclient m_pfunc_on_readmsg_tcp;
	pfunc_on_sendok m_pfunc_on_sendok;
	pfunc_on_highwatermark m_pfunc_on_highwatermark;
	WeakTcpConnectionPtr weak_conn_;
	std::atomic<bool> conn_status_;
	TcpClient client_;
	xTcpClientSinglePtr self_;
};


EventLoop* get_loop()
{
	static std::mutex lock_;
	std::lock_guard<std::mutex> lock(lock_);

	static EventLoopThreadPtr pLoopThr;
	static EventLoop* pLoop = NULL;
	if (!pLoopThr)
	{
		pLoopThr.reset(new EventLoopThread(NULL, "main_loop"));
		pLoop = pLoopThr->startLoop();
	}
	return pLoop;
}

class tag_cli_mgr
{
public:
	tag_cli_mgr()
		: m_pInvoker(NULL)
	{
		;
	}
	~tag_cli_mgr()
	{
		;
	}

	void add(xTcpClientSinglePtr cli_spr)
	{
		m_map_cli_spr[cli_spr] = false;
	}
	void clear()
	{
		for (auto& item : m_map_cli_spr)
		{
			auto cli_spr = item.first;
			if (cli_spr)
			{
				cli_spr->DisConnect();
			}
		}
		m_map_cli_spr.clear();
	}
	xTcpClientSinglePtr get_by_id(uint64_t cli_uuid)
	{
		for (auto& item : m_map_cli_spr)
		{
			auto cli_spr = item.first;
			if (cli_spr)
			{
				if (0 == cli_uuid)
					return cli_spr;

				auto uid = static_cast<uint64_t>(cli_spr->getConnuuid());
				if (uid == cli_uuid)
					return cli_spr;
			}
		}
		return nullptr;
	}
	void check()
	{
		int cnt(0);
		for (auto ite = m_map_cli_spr.begin(); ite != m_map_cli_spr.end();)
		{
			auto cli_spr = ite->first;
			if (cli_spr && !cli_spr->is_ok())
			{
				cli_spr->DisConnect();
				ite = m_map_cli_spr.erase(ite);
				cnt++;
				continue;
			}
			++ite;
		}

		for (int i = 0; i < cnt; ++i)
		{
			xTcpClientSinglePtr client_;
			InetAddress connect_ip_port(m_ip, m_port);
			client_.reset(new xTcpClientSingle(m_pInvoker, get_loop(), connect_ip_port,
				m_pFnOnConnect, m_pFnOnMessage, m_pFnOnSendOk, NULL,
				m_nDefRecvBuf, m_nMaxRecvBuf, m_nMaxSendQue));
			if (client_)
			{
				client_->Connect(client_);
				m_map_cli_spr[client_] = false;
			}
		}
	}
	void send(pfunc_on_heartbeat pFnOnHeartbeat)
	{
		if (!pFnOnHeartbeat)
			return;
		for (auto& item : m_map_cli_spr)
		{
			auto cli_spr = item.first;
			if (cli_spr && cli_spr->is_ok())
			{
				pFnOnHeartbeat(cli_spr->getConnuuid());
			}
		}
	}

protected:
	std::map<xTcpClientSinglePtr, bool> m_map_cli_spr;

public:
	void* m_pInvoker;			/*上层调用的类指针*/
	int m_nThreadNum;			/*线程数*/
	int m_idleconntime;		/*空连接踢空时间*/
	int m_nMaxTcpClient;		/*最大并发数*/
	std::string m_ip;
	uint16_t  m_port;
	pfunc_on_connection m_pFnOnConnect;
	pfunc_on_sendok m_pFnOnSendOk;
	pfunc_on_readmsg_tcpclient m_pFnOnMessage; 	/*与回应相关的事件回调(recv)*/
	size_t m_nDefRecvBuf;// = 4 * 1024,		/*默认接收buf*/
	size_t m_nMaxRecvBuf;// = 4 * 1024,		/*最大接收buf*/
	size_t m_nMaxSendQue;// = 512			/*最大发送队列*/

	size_t m_nMaxWnd; /*最大窗口大小*/
};


NetClient::NetClient() :
	m_climgr(new tag_cli_mgr()),
	m_httpClientPtr(new HttpClientManager())
{
}

NetClient::~NetClient()
{
}

bool NetClient::InitHttpClient(void* pInvoker, 
	int nThreadNum, 
	int idleconntime, 
	int nMaxHttpClient, 
	pfunc_on_msg_httpclient pFnOnMessage,
	size_t nDefRecvBuf,
	size_t nMaxRecvBuf,
	size_t nMaxSendQue)
{
	bool bRet = true;
	g_nMaxRecvBuf_Http = nMaxRecvBuf;
	m_httpClientPtr->Init(pInvoker, nThreadNum, idleconntime, nMaxHttpClient, pFnOnMessage, nDefRecvBuf, nMaxRecvBuf, nMaxSendQue);
	return bRet;
}

void NetClient::ExitHttpClient()
{
	m_httpClientPtr->Exit();
}

int NetClient::SendHttpRequest(uint64_t req_uuid, const boost::any& params, const HttpRequest& httpRequest)
{
	return m_httpClientPtr->Http_Request(req_uuid, params, httpRequest);;
}
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////


void ontimer(std::shared_ptr<NetClient> clispr)
{
	if (!clispr)
		return;
	std::lock_guard<std::mutex> lock(clispr->m_cli_lock);
	clispr->m_climgr->check();
}


void ontimer_heartbeat(std::shared_ptr<NetClient> clispr, pfunc_on_heartbeat pFnOnHeartbeat)
{
	if (!clispr)
		return;
	std::lock_guard<std::mutex> lock(clispr->m_cli_lock);
	clispr->m_climgr->send(pFnOnHeartbeat);
}

bool NetClient::InitTcpClient(void* pInvoker,		
	int nThreadNum,			
	int idleconntime,		
	int nMaxTcpClient,		
	std::string ip,
	uint16_t  port,
	pfunc_on_connection pFnOnConnect,
	pfunc_on_sendok pFnOnSendOk,
	pfunc_on_readmsg_tcpclient pFnOnMessage, 	
	size_t nDefRecvBuf,		
	size_t nMaxRecvBuf,		
	size_t nMaxSendQue,		
	uint64_t& uuid,			
	size_t nMaxWnd			)
{
	std::lock_guard<std::mutex> lock(m_cli_lock);


	// 测试
	// nMaxTcpClient = 1;

	bool flag_getit = false;
	for (int i = 0; i < nMaxTcpClient; ++i)
	{
		xTcpClientSinglePtr client_;
		InetAddress connect_ip_port(ip, port);
		client_.reset(new xTcpClientSingle(pInvoker, get_loop(), connect_ip_port,
			pFnOnConnect, pFnOnMessage, pFnOnSendOk, NULL,
			nDefRecvBuf, nMaxRecvBuf, nMaxSendQue));
		if (client_)
		{
			client_->Connect(client_);

			m_climgr->add(client_);
			flag_getit = true;
		}

	}

	if (flag_getit && m_climgr)
	{
		m_climgr->m_pInvoker = pInvoker;			/*上层调用的类指针*/
		m_climgr->m_nThreadNum = nThreadNum;			/*线程数*/
		m_climgr->m_idleconntime = idleconntime;		/*空连接踢空时间*/
		m_climgr->m_nMaxTcpClient = nMaxTcpClient;		/*最大并发数*/
		m_climgr->m_ip = ip;
		m_climgr->m_port = port;
		m_climgr->m_pFnOnConnect = pFnOnConnect;
		m_climgr->m_pFnOnSendOk = pFnOnSendOk;
		m_climgr->m_pFnOnMessage = pFnOnMessage; 	/*与回应相关的事件回调(recv)*/
		m_climgr->m_nDefRecvBuf = nDefRecvBuf;// = 4 * 1024,		/*默认接收buf*/
		m_climgr->m_nMaxRecvBuf = nMaxRecvBuf;// = 4 * 1024,		/*最大接收buf*/
		m_climgr->m_nMaxSendQue = nMaxSendQue;// = 512			/*最大发送队列*/
		m_climgr->m_nMaxWnd = nMaxWnd;
	}

	auto loop = get_loop();
	if (loop)
		loop->runEvery(5.0, std::bind(ontimer, shared_from_this()));

	return flag_getit;
}

int  NetClient::SendTcpRequest(uint64_t cli_uuid, uint64_t cnn_uuid, const boost::any& params, const char* szMsg, size_t nMsgLen)
{
	std::lock_guard<std::mutex> lock(m_cli_lock);

	auto client_ = m_climgr->get_by_id(cli_uuid);
	if (client_)
	{
		client_->SendTcpRequest(params, szMsg, nMsgLen, true);
	}

	return 0;
}


void NetClient::ExitTcpClient(uint64_t cli_uuid)
{
	std::lock_guard<std::mutex> lock(m_cli_lock);
	m_climgr->clear();
	return;
}

void NetClient::SetHeartbeat(pfunc_on_heartbeat pFnOnHeartbeat, double interval)
{
	std::lock_guard<std::mutex> lock(m_cli_lock);
	if (interval <= 0)
		interval = 10.0;

	auto loop = get_loop();
	if (loop)
	{
		if (!m_timerid_heartbeat.empty())
		{
			loop->cancel(boost::any_cast<TimerId>(m_timerid_heartbeat));
		}
		auto timerid = loop->runEvery(interval, std::bind(ontimer_heartbeat, shared_from_this(), pFnOnHeartbeat));
		m_timerid_heartbeat = timerid;
	}
	
}
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////


bool NetClient::InitTcpClient_bak(void* pInvoker,	
	int nThreadNum,			
	int idleconntime,		
	int nMaxTcpClient,		
	std::string ip,
	uint16_t  port,
	pfunc_on_connection pFnOnConnect,
	pfunc_on_sendok pFnOnSendOk,
	pfunc_on_readmsg_tcpclient pFnOnMessage, 	
	size_t nDefRecvBuf,		
	size_t nMaxRecvBuf,		
	size_t nMaxSendQue,		
	uint64_t& uuid,			
	size_t nMaxWnd			)
{
	{
		static std::atomic<bool> init_flag(false);
		if (false == init_flag)
		{
			init_flag = true;
			md_cli_init(nThreadNum);
		}

	}

	auto cli_spr = get_md_cli(static_cast<uint16_t>(nMaxTcpClient));
	if (cli_spr)
	{
		auto inst = std::make_shared<NetTcpClientMgr>();
		if (!inst)
			return false;

		inst->m_cb_spr = std::make_shared<NetTcpClientMgrCallback>();
		inst->m_pInvokePtr = pInvoker;

		NetTcpClientMgrContaner::instance().m_tcpclient_connect_cb = pFnOnConnect;
		NetTcpClientMgrContaner::instance().m_tcpclient_readmsg_cb = pFnOnMessage;
		NetTcpClientMgrContaner::instance().m_tcpclient_send_cb = pFnOnSendOk;

		inst->m_cli_spr = cli_spr;
		inst->m_cli_spr->set_callback(pInvoker, inst->m_cb_spr);
		inst->m_cli_spr->set_buf_info(nDefRecvBuf, nMaxRecvBuf, nMaxSendQue);

		inst->m_nIdelTime = idleconntime;
		inst->m_clients = nMaxTcpClient;
		inst->m_wnd_size = static_cast<int>(nMaxWnd);

		inst->m_nDefRecvBuf = nDefRecvBuf;
		inst->m_nMaxRecvBuf = nMaxRecvBuf;
		inst->m_nMaxSendQue = nMaxSendQue;

		auto ret_ = cli_spr->conn(ip, port);
		// Ĭ���Զ�����
		inst->m_cli_spr->set_auto_reconnect(true);

		auto cur_uuid = NetTcpClientMgrContaner::instance().add(inst);
		if (0 == cur_uuid)
			return false;

		uuid = cur_uuid;
		return ret_;
	}

	return true;
}

int  NetClient::SendTcpRequest_bak(uint64_t cli_uuid, uint64_t cnn_uuid, const boost::any& params, const char* szMsg, size_t nMsgLen)
{
	auto cli = NetTcpClientMgrContaner::instance().get_cli(cli_uuid, cnn_uuid);
	if (cli)
	{
		return cli->send(cnn_uuid, params, szMsg, nMsgLen);
	}
	else
		return -1001;

	// 	if (NetTcpClientMgr::instance().m_cli_spr)
	// 	{
	// 		return NetTcpClientMgr::instance().m_cli_spr->send(req_uuid, params, szMsg, nMsgLen);
	// 	}
	// 	else
	// 		return -1001;

	return 0;
}



// cli_uuid = 0 ʱ��ȫ�����
void NetClient::ExitTcpClient_bak(uint64_t cli_uuid)
{
	NetTcpClientMgrContaner::instance().del(cli_uuid);


	// 	if (NetTcpClientMgr::instance().m_cli_spr)
	// 	{
	// 		NetTcpClientMgr::instance().m_cli_spr->stop();
	// 		std::this_thread::sleep_for(std::chrono::seconds(2));
	// 		md_cli_uninit();
	// 		return;
	// 
	// 	}

	return;
}

//////////////////////////////////////////////////////////////////////////

XTcpClient::XTcpClient()
	: m_pInvokePtr(NULL)
{
	m_mwcli_spr = std::make_shared<MwCli>();
}

//��ʼ���ӿ�
extern std::mutex g_mwtcp_lock;
int XTcpClient::Init(const void* pInvokePtr, const Params& params, CallBackRecv cb)
{
	std::lock_guard<std::mutex> lock(g_mwtcp_lock);
	m_mwcli_spr->m_params = params;
	m_mwcli_spr->m_mw_cb = cb;
	m_mwcli_spr->m_pInvokePtr = pInvokePtr;
	m_mwcli_spr->m_cb_spr = std::make_shared<MwTcpClientCallback>();
	if (!m_mwcli_spr->m_cb_spr)
		return -1;

	// ���ûص�����Ϣ
	m_mwcli_spr->m_cb_spr->set_mwcli_spr(m_mwcli_spr);

	int nThreads = m_mwcli_spr->m_params.nThreads;
	if (nThreads <= 0)
		nThreads = 1;

	static int x = 0;
	if (0 == x)
	{
		x = 1;
		md_cli_init(nThreads);
	}
	return 0;
}
//����ʼ���ӿ�
int XTcpClient::UnInit()
{
// 	std::lock_guard<std::mutex> lock(g_mwtcp_lock);
// 	return md_cli_uninit();
	return 0;
}
//����ʱ���ò���
void XTcpClient::SetRunParam(const Params& params)
{
	std::lock_guard<std::mutex> lock(g_mwtcp_lock);

	m_mwcli_spr->m_params = params;

	// ��������,Ҫ����ʵ���£���Ҫ�ϲ��ֶ��������� ReConnLink

	return;
}
//��������, > 0 nLinkId, ����ʧ��, moder��ͬ���첽��, 
int XTcpClient::ApplyNewLink(const std::string ip, int port, int moder, int ms_timeout)
{
	std::shared_ptr<NetTcpClientInterface> cli_spr;
	std::shared_ptr<MwCli::mwcli_info> mwcli_info_spr;

	int fd = -1;
	{
		std::lock_guard<std::mutex> lock(g_mwtcp_lock);
		{
			// ��ȡ����fd
			static std::atomic<int> nNodeNum(0);
			static int max_fd = 62000;
			int max_loop = max_fd;
			while (max_loop--)
			{
				nNodeNum++;
				if (nNodeNum >= max_fd)
					nNodeNum = 1;

				auto ite = m_mwcli_spr->m_map_index_cli_spr.find(nNodeNum);
				if (ite == m_mwcli_spr->m_map_index_cli_spr.end())
				{
					fd = nNodeNum;
					break;
				}
			}
		}
		if (fd < 0)
			return fd;

		// ��ȡ����
		int iClientsPerNode = 1;
		cli_spr = get_md_cli(static_cast<uint16_t>(iClientsPerNode), m_mwcli_spr->m_params.nWnd);
		if (!cli_spr)
			return -1;

		mwcli_info_spr = std::make_shared<MwCli::mwcli_info>(cli_spr, fd, ip, port, moder, ms_timeout);
		if (!mwcli_info_spr)
			return -1;

		int nNodeNum = fd;
		m_mwcli_spr->m_map_index_cli_spr[nNodeNum] = mwcli_info_spr;
		cli_spr->set_callback(m_mwcli_spr->m_pInvokePtr, m_mwcli_spr->m_cb_spr);
		// ��������
		// TODO: nAutoReconTime����漰�޸�muduo�Դ����������ƣ���ʱ����
		cli_spr->set_auto_reconnect(m_mwcli_spr->m_params.bAutoRecon);
		cli_spr->set_node_num(nNodeNum);
		cli_spr->set_buf_info(m_mwcli_spr->m_params.nDefBuf,
			m_mwcli_spr->m_params.nMaxBuf,
			m_mwcli_spr->m_params.nMaxQue);
	}

	// ���ӷ����
	if (cli_spr)
	{
		if (0 == moder)
		{
			// �첽
			cli_spr->conn(ip, static_cast<uint16_t>(port));
		}
		else
		{
			// ͬ��
			cli_spr->conn(ip, static_cast<uint16_t>(port));
			if (mwcli_info_spr)
			{
				std::unique_lock<std::mutex> lock(mwcli_info_spr->m_notify_lock);
				std::cv_status ret = mwcli_info_spr->m_notify.wait_for(lock, std::chrono::milliseconds(ms_timeout));
				if (ret == std::cv_status::timeout)
					std::cout << "m_notify.wait_for timeout:" << ms_timeout << "(ms)" << std::endl;
				else
					std::cout << "m_notify.wait_for notifyed:" << ms_timeout << "(ms)" << std::endl;
			}
		}
	}

	return fd;
}


//�ر�����
int XTcpClient::CloseLink(int nLinkId)
{
	std::lock_guard<std::mutex> lock(g_mwtcp_lock);
	auto ite = m_mwcli_spr->m_map_index_cli_spr.find(nLinkId);
	if (ite != m_mwcli_spr->m_map_index_cli_spr.end())
	{
		if (ite->second
			&& ite->second->cli_spr)
			ite->second->cli_spr->stop();
		m_mwcli_spr->m_map_index_cli_spr.erase(ite);
	}
	return 0;
}
//��������
int XTcpClient::ReConnLink(int nLinkId)
{
	std::shared_ptr<NetTcpClientInterface> cli_spr;
	std::shared_ptr<MwCli::mwcli_info> mwcli_info_spr;
	std::shared_ptr<MwCli::mwcli_info> item;

	{
		std::lock_guard<std::mutex> lock(g_mwtcp_lock);

		auto ite = m_mwcli_spr->m_map_index_cli_spr.find(nLinkId);
		if (ite != m_mwcli_spr->m_map_index_cli_spr.end())
		{
			if (ite->second
				&& ite->second->cli_spr)
			{
				ite->second->cli_spr->stop();
				item = ite->second;
			}

			m_mwcli_spr->m_map_index_cli_spr.erase(ite);
		}

		// ��ȡ����
		int iClientsPerNode = 1;
		cli_spr = get_md_cli(static_cast<uint16_t>(iClientsPerNode), m_mwcli_spr->m_params.nWnd);
		if (!cli_spr)
			return -1;

		mwcli_info_spr = std::make_shared<MwCli::mwcli_info>(cli_spr, item->nLinkId, item->ip, item->port, item->moder, item->ms_timeout);
		if (!mwcli_info_spr)
			return -1;

		int nNodeNum = nLinkId;
		m_mwcli_spr->m_map_index_cli_spr[nNodeNum] = mwcli_info_spr;
		cli_spr->set_callback(m_mwcli_spr->m_pInvokePtr, m_mwcli_spr->m_cb_spr);

		// ��������
		cli_spr->set_auto_reconnect(m_mwcli_spr->m_params.bAutoRecon);
		cli_spr->set_node_num(nNodeNum);
		cli_spr->set_buf_info(m_mwcli_spr->m_params.nDefBuf,
			m_mwcli_spr->m_params.nMaxBuf,
			m_mwcli_spr->m_params.nMaxQue);

	}

	// ���ӷ����
	if (cli_spr && mwcli_info_spr)
	{
		if (0 == mwcli_info_spr->moder)
		{
			// �첽
			cli_spr->conn(mwcli_info_spr->ip, static_cast<uint16_t>(mwcli_info_spr->port));
		}
		else
		{
			// ͬ��
			cli_spr->conn(mwcli_info_spr->ip, static_cast<uint16_t>(mwcli_info_spr->port));
			if (mwcli_info_spr)
			{
				std::unique_lock<std::mutex> lock(mwcli_info_spr->m_notify_lock);
				mwcli_info_spr->m_notify.wait_for(lock, std::chrono::milliseconds(mwcli_info_spr->ms_timeout));
			}
		}
	}

	return 0;

}
//����״̬
bool XTcpClient::LinkState(int nLinkId)
{
	std::lock_guard<std::mutex> lock(g_mwtcp_lock);
	auto ite = m_mwcli_spr->m_map_index_cli_spr.find(nLinkId);
	if (ite != m_mwcli_spr->m_map_index_cli_spr.end())
	{
		if (ite->second
			&& ite->second->cli_spr)
		{
			return ite->second->cli_spr->state();
		}
	}
	return false;
}
//��ȡĳ�����ӵĵ�ǰ�����С
size_t XTcpClient::GetWaitBufSize(int nLinkId)
{
	std::lock_guard<std::mutex> lock(g_mwtcp_lock);
	auto ite = m_mwcli_spr->m_map_index_cli_spr.find(nLinkId);
	if (ite != m_mwcli_spr->m_map_index_cli_spr.end())
	{
		if (ite->second
			&& ite->second->cli_spr)
		{
			return ite->second->cli_spr->recvbuf_size();
		}
	}
	return 0;
}
//��ȡ���л����С
size_t XTcpClient::GetAllWaitBufSize()
{
	size_t bufsize = 0;
	std::lock_guard<std::mutex> lock(g_mwtcp_lock);
	for (auto& item : m_mwcli_spr->m_map_index_cli_spr)
	{
		if (item.second
			&& item.second->cli_spr)
		{
			bufsize += item.second->cli_spr->recvbuf_size();
		}
	}
	return bufsize;
}
//��ȡ��ǰ��ʹ�ö��ٴ���
size_t XTcpClient::GetUseWnd(int nLinkId)
{
	size_t bufsize = 0;
	std::lock_guard<std::mutex> lock(g_mwtcp_lock);
	for (auto& item : m_mwcli_spr->m_map_index_cli_spr)
	{
		if (item.second
			&& item.second->cli_spr)
		{
			bufsize += item.second->cli_spr->cur_wnd_size();
		}
	}
	return bufsize;
}
//���ͽӿ�
int XTcpClient::Send(int nLinkId, const char* data, size_t len)
{
	std::lock_guard<std::mutex> lock(g_mwtcp_lock);
	auto ite = m_mwcli_spr->m_map_index_cli_spr.find(nLinkId);
	if (ite != m_mwcli_spr->m_map_index_cli_spr.end()
		&& ite->second
		&& ite->second->cli_spr)
	{
		return ite->second->cli_spr->send(data, len);
	}
	else
		return -1001;

	return 0;
}
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////


}
}
