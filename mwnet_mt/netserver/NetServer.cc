#include "NetServer.h"
#include <mwnet_mt/net/TcpServer.h>
#include <mwnet_mt/base/LogFile.h>
#include <mwnet_mt/base/Logging.h>
#include <mwnet_mt/base/Thread.h>
#include <mwnet_mt/base/ThreadLocalSingleton.h>
#include <mwnet_mt/net/EventLoop.h>
#include <mwnet_mt/net/EventLoopThreadPool.h>
#include <mwnet_mt/net/InetAddress.h>
#include <mwnet_mt/net/httpparser/httpparser.h>
#include <mwnet_mt/base/CountDownLatch.h>

//#include <mwnet_mt/util/MWLogger.h>

#include <boost/ptr_container/ptr_vector.hpp>
#include <boost/circular_buffer.hpp>
#include <boost/unordered_set.hpp>
#include <boost/version.hpp>
#include <functional>
#include <memory>
#include <utility>
#include <errno.h>
#include <stdio.h>
#include <stdarg.h>
#include <unistd.h>
#include <vector>
#include <list>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
#include <atomic>
#include <mutex>


using namespace mwnet_mt;
using namespace mwnet_mt::net;
using namespace mw_http_parser;
//using namespace MWLOGGER;

namespace MWNET_MT
{
namespace SERVER
{
typedef std::weak_ptr<TcpConnection> WeakTcpConnectionPtr;
typedef std::shared_ptr<HttpRequest> HttpRequestPtr;
typedef std::shared_ptr<HttpResponse> HttpResponsePtr;
typedef std::shared_ptr<HttpParser> HttpParserPtr;
typedef std::list<WeakTcpConnectionPtr> WeakConnectionList;

std::string stdstring_format(const char *fmt, ...)
{
	std::string strRet = "";
	va_list marker;
	va_start(marker, fmt);
	int nLength = vsnprintf(NULL, 0, fmt, marker);
	va_end(marker);
	if (nLength < 0) return "";
	va_start(marker, fmt);
	strRet.resize(nLength + 1);
	int nSize = vsnprintf(const_cast<char* >(strRet.data()), nLength + 1, fmt, marker);
	va_end(marker);
	if (nSize > 0)
	{
		strRet.resize(nSize);
	}
	else
	{
		strRet = "";
	}
	return strRet;
}

void stdstring_format(std::string& strRet, const char *fmt, ...)
{
	va_list marker;
	va_start(marker, fmt);
	int nLength = vsnprintf(NULL, 0, fmt, marker);
	va_end(marker);
	if (nLength < 0) return;
	va_start(marker, fmt);
	strRet.resize(nLength + 1);
	int nSize = vsnprintf(const_cast<char* >(strRet.data()), nLength + 1, fmt, marker);
	va_end(marker);
	if (nSize > 0)
	{
		strRet.resize(nSize);
	}
	else
	{
		strRet = "";
	}
}

// 十六进制字符转换成整数
int HexchartoInt(char chHex)
{
	int iDec = 0;
	switch (chHex)
	{
	case '0':
	case '1':
	case '2':
	case '3':
	case '4':
	case '5':
	case '6':
	case '7':
	case '8':
	case '9':
		iDec = chHex - '0';
		break;
	case 'A':
	case 'B':
	case 'C':
	case 'D':
	case 'E':
	case 'F':
		iDec = chHex - 'A' + 10;
		break;
	case 'a':
	case 'b':
	case 'c':
	case 'd':
	case 'e':
	case 'f':
		iDec = chHex - 'a' + 10;
		break;
	default:
		iDec = 0;
		break;
	}
	return iDec;
}

void BinToHexstr(const unsigned char *src, size_t srcLen, std::string& dest)
{
	char tmp[3] = {0};
	dest.reserve(srcLen*2);

	for(size_t i = 0; i < srcLen; i++)
	{
		sprintf(tmp, "%02x", src[i]&0xff);
		dest.append(tmp);
	}
}

int HexStrToBin(unsigned char *pDest, int nDestSize, const std::string& strSrc)
{
	size_t nReadLen = strSrc.length();
	for (size_t i = 0, k = 0; i < nReadLen && nDestSize > 0; i += 2,--nDestSize)
	{
		pDest[k++] = static_cast<unsigned char>(HexchartoInt(strSrc[i]) * 16 + HexchartoInt(strSrc[i+1]));
	}
	return static_cast<int>(nReadLen / 2);
}


class ConnNode : public mwnet_mt::copyable
{
public:
	ConnNode(char index)
		: m_index(index)
	{
		time(&m_tLastUpd);
	}
	~ConnNode()
	{
	}
public:
	void UpdTm()
	{
		time(&m_tLastUpd);
	}
	void UpdPos(const WeakConnectionList::iterator& pos)
	{
		m_nodepos = pos;
	}
	int TmDiff()
	{
		return static_cast<int>(time(NULL) - m_tLastUpd);
	}
	char GetIndex() const
	{
		return m_index;
	}
	const WeakConnectionList::iterator& GetPos() const
	{
		return m_nodepos;
	}
private:
	char m_index;							// server索引号
	time_t m_tLastUpd;						// 最后一次活动时间
	WeakConnectionList::iterator m_nodepos;	// 连表中的位置
};
typedef std::shared_ptr<ConnNode> ConnNodePtr;
/*
enum MODULE_LOGS
{
	MODULE_LOGS_DEFAULT,	//默认模块,如主模块
	MODULE_LOG_NUMS,		//日志个数
};

const char* g_module_logs[] = 
{
"syslog",
"download"
};
*/
typedef struct _net_ctrl_params
{
	//-----------------------------------------------------------
	// 全局变量
	int g_nMaxConnNum_Tcp;				// 支持的最大连接数
	AtomicInt32 g_nCurrConnNum_Tcp; 	// 当前连接数
	AtomicInt64 g_nTotalReqNum_Tcp; 	// 总的请求数
	AtomicInt64 g_nLastTotalReqNum_Tcp; // 上次的总请求数
	MutexLock g_mapTcpConns_lock;
	std::map<uint64_t, WeakTcpConnectionPtr> g_mapTcpConns;

	int g_nMaxConnNum_Http; 				// 支持的最大连接数
	AtomicInt32 g_nCurrConnNum_Http;		// 当前连接数
	AtomicInt64 g_nTotalReqNum_Http;		// 总的请求数
	AtomicInt64 g_nLastTotalReqNum_Http;	// 上次的总请求数
	uint16_t g_unique_node_id;

	AtomicInt64 g_nSequnceId;
	MutexLock g_mapHttpConns_lock;
	std::map<uint64_t, WeakTcpConnectionPtr> g_mapHttpConns;

	bool g_bEnableRecvLog_http;					//接收日志开关_http
	bool g_bEnableSendLog_http;					//发送日志开关_http
	bool g_bEnableRecvLog_tcp;					//接收日志开关_tcp
	bool g_bEnableSendLog_tcp;					//发送日志开关_tcp

	size_t g_nMaxRecvBuf_Tcp;					//tcp最大接收缓冲
	size_t g_nMaxRecvBuf_Http;					//http最大接收缓冲

	AtomicInt64 g_nTotalWaitSendNum_http;		//http等待发送出的包的数量
	AtomicInt64 g_nTotalWaitSendNum_tcp;		//tcp等待发送出的包的数量

	bool g_bExit_http;							//http服务退出标志
	bool g_bExit_tcp;							//http服务退出标志
}net_ctrl_params;

typedef std::shared_ptr<net_ctrl_params> net_ctrl_params_ptr;

//-----------------------------------------------------------

//日志
//..........................................................
std::unique_ptr<mwnet_mt::LogFile> g_logFile;

void outputFunc(const char* msg, int len)
{
  g_logFile->append(msg, len);
}

void flushFunc()
{
  g_logFile->flush();
}
//........................................................

uint64_t MakeConnuuid(uint16_t unique_node_id, AtomicInt64& nSequnceId)
{
	unsigned int nYear = 0;
	unsigned int nMonth = 0;
	unsigned int nDay = 0;
	unsigned int nHour = 0;
	unsigned int nMin = 0;
	unsigned int nSec = 0;
	unsigned int nNodeid = unique_node_id;
	unsigned int nNo = static_cast<unsigned int>(nSequnceId.getAndAdd(1) % (0x03ffff));

  	struct timeval tv;
	struct tm	   tm_time;
	gettimeofday(&tv, NULL);
	localtime_r(&tv.tv_sec, &tm_time);

/*
  	time_t t = time(NULL);
  	struct tm tm_time = *localtime(&t);
*/
	nYear = tm_time.tm_year+1900;
	nYear = nYear%100;
	nMonth = tm_time.tm_mon+1;
	nDay = tm_time.tm_mday;
	nHour = tm_time.tm_hour;
	nMin = tm_time.tm_min;
	nSec = tm_time.tm_sec;

	int64_t j = 0;
	j |= static_cast<int64_t>(nYear& 0x7f) << 57;   //year 0~99
	j |= static_cast<int64_t>(nMonth & 0x0f) << 53;//month 1~12
	j |= static_cast<int64_t>(nDay & 0x1f) << 48;//day 1~31
	j |= static_cast<int64_t>(nHour & 0x1f) << 43;//hour 0~24
	j |= static_cast<int64_t>(nMin & 0x3f) << 37;//min 0~59
	j |= static_cast<int64_t>(nSec & 0x3f) << 31;//second 0~59
	j |= static_cast<int64_t>(nNodeid & 0x01fff) << 18;//nodeid 1~8000
	j |= static_cast<int64_t>(nNo & 0x03ffff);	//seqid,0~0x03ffff

	return j;
}

//----------------------------------------------------TcpServer Begin---------------------------------------------------------------
class TcpNetServerImpl
{
public:
	TcpNetServerImpl(void* pInvoker,char index,EventLoop* loop,const InetAddress& listenAddr,int threadnum,int idleconntime,bool bReusePort,
		pfunc_on_connection pOnConnection, pfunc_on_readmsg_tcp pOnReadMsg, pfunc_on_sendok pOnSendOk, 
		pfunc_on_senderr pOnSendErr, pfunc_on_highwatermark pOnHighWaterMark,
		size_t nDefRecvBuf, size_t nMaxRecvBuf, size_t nMaxSendQue, net_ctrl_params_ptr& ptrParams)
	:	m_pInvoker(pInvoker),
		m_index(index),
		m_loop(loop),
		m_server(loop,stdstring_format("TcpSvr%02d_%02d", index+1, loop->getidinpool()).c_str(),listenAddr,bReusePort?TcpServer::kReusePort:TcpServer::kNoReusePort, nDefRecvBuf, nMaxRecvBuf, nMaxSendQue),
		m_nIdleConnTime(idleconntime),
		m_nHighWaterMark(nMaxSendQue),
		m_pfunc_on_connection(pOnConnection),
		m_pfunc_on_readmsg_tcp(pOnReadMsg),
		m_pfunc_on_sendok(pOnSendOk),
		m_pfunc_on_senderr(pOnSendErr),
		m_pfunc_on_highwatermark(pOnHighWaterMark),
		m_ptrCtrlParams(ptrParams)		
	{
		// 绑定连接事件回调(connect,disconnect)
		m_server.setConnectionCallback(std::bind(&TcpNetServerImpl::onConnection, this, _1));
		// 绑定收到的数据回调
		m_server.setMessageCallback(std::bind(&TcpNetServerImpl::onMessage, this, _1, _2, _3));
		// 绑定写数据完成回调
		m_server.setWriteCompleteCallback(std::bind(&TcpNetServerImpl::onWriteOk, this, _1, _2));
		// 绑定写数据异常或失败回调
		m_server.setWriteErrCallback(std::bind(&TcpNetServerImpl::onWriteErr, this, _1, _2, _3));
		// 绑定线程启动成功回调
		m_server.setThreadInitCallback(std::bind(&TcpNetServerImpl::onThreadInit, this, _1));
		// 设置工作线程数
		m_server.setThreadNum(threadnum);
		// 启动server
		m_loop->runInLoop(std::bind(&TcpNetServerImpl::start, this));
	}

	// 重置空连接踢空时间
	void ResetIdleConnTime(int idleconntime)
	{
		m_nIdleConnTime = idleconntime;
	}

	// 发送信息
	int SendMsgWithTcpServer(const TcpConnectionPtr& conn, const boost::any& params, const char* szMsg, size_t nMsgLen, int timeout, bool bKeepAlive)
	{
		if (!bKeepAlive) conn->setNeedCloseFlag();
		
		return conn->send(szMsg, static_cast<int>(nMsgLen), params, timeout);
	}

	// 强制关闭连接
	void ForceCloseConnection(const TcpConnectionPtr& conn)
	{
		conn->shutdown();
		//conn->forceCloseWithDelay(1.0);  // > round trip of the whole Internet.
		conn->forceClose();
	}
	
	// 设置tcp会话的信息(请传session类的智能指针，并且只允许调用一次&session类中自行控制多线程互斥问题)
	void SetTcpSessionInfo(const TcpConnectionPtr& conn, const boost::any& sessioninfo)
	{
		conn->setSessionData(sessioninfo);
	}

	// 设置高水位值
	void SetTcpHighWaterMark(const TcpConnectionPtr& conn, size_t highWaterMark)
	{
		conn->setHighWaterMark(highWaterMark);
	}

	void StopTcpServer()
	{
		m_server.stop();
	}
private:
 	void AddToConnList(const TcpConnectionPtr& conn)
 	{
 		conn->setTcpNoDelay(true);

 		ConnNodePtr node(new ConnNode(m_index));
		node->UpdTm();
		WeakConnectionList& connlist = ThreadLocalSingleton<WeakConnectionList>::instance();
		connlist.push_back(conn);
		node->UpdPos(--connlist.end());
    	conn->setContext(node);

		if (m_ptrCtrlParams->g_unique_node_id >= 1 && m_ptrCtrlParams->g_unique_node_id <= 8000)
		{
			uint64_t conn_uuid = MakeConnuuid(m_ptrCtrlParams->g_unique_node_id, m_ptrCtrlParams->g_nSequnceId);
			conn->setConnuuid(conn_uuid);
			WeakTcpConnectionPtr weakconn(conn);
			MutexLockGuard lock(m_ptrCtrlParams->g_mapTcpConns_lock);
			m_ptrCtrlParams->g_mapTcpConns.insert(std::make_pair(conn_uuid, weakconn));
		}
 	}

	void UpdateConnList(const TcpConnectionPtr& conn, bool bDel)
 	{
 		if (conn && !conn->getContext().empty())
 		{
	 		ConnNodePtr node(boost::any_cast<ConnNodePtr>(conn->getContext()));
			if (node)
			{
				WeakConnectionList& connlist = ThreadLocalSingleton<WeakConnectionList>::instance();
				if (!bDel)
				{
					node->UpdTm();
					connlist.splice(connlist.end(), connlist, node->GetPos());
					node->UpdPos(--connlist.end());
				}
				else
				{
					if (m_ptrCtrlParams->g_unique_node_id >= 1 && m_ptrCtrlParams->g_unique_node_id <= 8000)
					{
						MutexLockGuard lock(m_ptrCtrlParams->g_mapTcpConns_lock);
						m_ptrCtrlParams->g_mapTcpConns.erase(conn->getConnuuid());
					}
					WeakConnectionList::iterator it = node->GetPos();
	    			if (!connlist.empty() && it != connlist.end())
					{
						connlist.erase(it);
						//置为无效pos
						node->UpdPos(connlist.end());
	    			}
				}
			}
 		}
 	}
private:
 	void start()
  	{
    	m_server.start();
  	}

	void onHighWater(const TcpConnectionPtr& conn, size_t highWaterMark)
	{
		if (!conn) return;

		int nRet = 0;
		if (m_pfunc_on_highwatermark)
		{
			WeakTcpConnectionPtr weakconn(conn);
			nRet = m_pfunc_on_highwatermark(m_pInvoker, conn->getConnuuid(), weakconn, conn->getSessionData(), highWaterMark);
		}

		if (nRet < 0 || conn->bNeedClose())
		{
			conn->shutdown();
			//conn->forceCloseWithDelay(1.0);  // > round trip of the whole Internet.
			conn->forceClose();
		}
	}
	
	void onConnection(const TcpConnectionPtr& conn)
	{
		if (!conn) return;

		if (conn->connected())
		{
			conn->setHighWaterMarkCallback(std::bind(&TcpNetServerImpl::onHighWater, this, _1, _2), m_nHighWaterMark);
			
			AddToConnList(conn);

			// 限制最大连接数
			if (m_ptrCtrlParams->g_nCurrConnNum_Tcp.incrementAndGet() > m_ptrCtrlParams->g_nMaxConnNum_Tcp)
		    {
		      	conn->shutdown();
		      	//conn->forceCloseWithDelay(1);  // > round trip of the whole Internet.
		      	conn->forceClose();
				LOG_INFO << "TcpServer reached the maxconnnum:" << m_ptrCtrlParams->g_nMaxConnNum_Tcp;
		      	return;
		    }

			if (m_pfunc_on_connection)
			{
				WeakTcpConnectionPtr weakconn(conn);
				int nRet = m_pfunc_on_connection(m_pInvoker, conn->getConnuuid(), weakconn, conn->getSessionData(), true, conn->peerAddress().toIpPort().c_str());
				if (nRet < 0)
				{
					conn->shutdown();
					//conn->forceCloseWithDelay(1.0);  // > round trip of the whole Internet.
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
			m_ptrCtrlParams->g_nCurrConnNum_Tcp.decrement();

			if (m_pfunc_on_connection)
			{
				WeakTcpConnectionPtr weakconn(conn);
				m_pfunc_on_connection(m_pInvoker, conn->getConnuuid(), weakconn, conn->getSessionData(), false, conn->peerAddress().toIpPort().c_str());
			}

			UpdateConnList(conn, true);
		}
	}

	void onMessage(const TcpConnectionPtr& conn, Buffer* buf, Timestamp time)
	{
		if (!conn) return;

		m_ptrCtrlParams->g_nTotalReqNum_Tcp.increment();

		UpdateConnList(conn, false);

		if (m_pfunc_on_readmsg_tcp)
		{
			int nReadedLen = 0;
			StringPiece RecvMsg = buf->toStringPiece();

			WeakTcpConnectionPtr weakconn(conn);
			int nRet = m_pfunc_on_readmsg_tcp(m_pInvoker, conn->getConnuuid(), weakconn, conn->getSessionData(), RecvMsg.data(), RecvMsg.size(), nReadedLen);
			// 刚好都是完整包
			if (nReadedLen == RecvMsg.size())
			{
				buf->retrieveAll();
			}
			// 粘包/残包
			else
			{
				// 将buf中的指针移至当前已处理到的地方
				buf->retrieve(nReadedLen);

				// 若没有超过最大接收BUF,不作任何处理...等接完整了再处理,若超过,则直接close
				if (buf->readableBytes() >= m_ptrCtrlParams->g_nMaxRecvBuf_Tcp)
				{
					nRet = -1;
					LOG_WARN<<"[TCP][RECV]["<<conn->getConnuuid()<<"]\nillegal data size:"<<buf->readableBytes()<<",maybe too long\n";
				}
			}
			// 判断返回值，并做不同的处理
			if (nRet < 0)
			{
				conn->shutdown();
				//conn->forceCloseWithDelay(1.0);  // > round trip of the whole Internet.
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

		// 减少总待发出计数
		m_ptrCtrlParams->g_nTotalWaitSendNum_tcp.decrement();

		int nRet = 0;
		if (m_pfunc_on_sendok)
		{
			WeakTcpConnectionPtr weakconn(conn);
			nRet = m_pfunc_on_sendok(m_pInvoker, conn->getConnuuid(), weakconn, conn->getSessionData(), params);
		}

		if (nRet < 0 || conn->bNeedClose())
		{
			conn->shutdown();
			//conn->forceCloseWithDelay(1.0);  // > round trip of the whole Internet.
			conn->forceClose();
		}
	}

	void onWriteErr(const TcpConnectionPtr& conn, const boost::any& params, int errCode)
	{
		if (!conn) return;

		// 减少总待发出计数
		m_ptrCtrlParams->g_nTotalWaitSendNum_tcp.decrement();

		int nRet = 0;
		if (m_pfunc_on_senderr)
		{
			WeakTcpConnectionPtr weakconn(conn);
			nRet = m_pfunc_on_senderr(m_pInvoker, conn->getConnuuid(), weakconn, conn->getSessionData(), params, errCode);
		}

		if (nRet < 0 || conn->bNeedClose())
		{
			conn->shutdown();
			//conn->forceCloseWithDelay(1.0);  // > round trip of the whole Internet.
			conn->forceClose();
		}
	}

	void onThreadInit(EventLoop* loop)
	{
		m_server.getLoop()->runInLoop(std::bind(&TcpNetServerImpl::startTimer, this));
	}

	// 开启定时器
	void startTimer()
	{
		EventLoop* loop = m_server.threadPool()->getNextLoop();
		loop->runInLoop(std::bind(&TcpNetServerImpl::InitConnList, this));
		loop->runEvery(1.0, std::bind(&TcpNetServerImpl::onCheckConnTimeOut, this));
	}

	// 初始化连接队列
	void InitConnList()
	{
		ThreadLocalSingleton<WeakConnectionList>::instance();
	}

	// 定时器,定时检测超时
	void onCheckConnTimeOut()
	{
		WeakConnectionList& connlist = ThreadLocalSingleton<WeakConnectionList>::instance();
		for (WeakConnectionList::iterator it = connlist.begin(); it != connlist.end();)
		{
			TcpConnectionPtr conn(it->lock());
			if (conn && !conn->getContext().empty())
			{
				ConnNodePtr node(boost::any_cast<ConnNodePtr>(conn->getContext()));
				if (node)
				{
					int nTmDiff = node->TmDiff();
					// 超时
					if (nTmDiff > m_nIdleConnTime)
					{
						if (conn->connected())
						{
							LOG_INFO<<"IP:["<<conn->peerAddress().toIpPort()<<"] CONNUUID:["<<conn->getConnuuid()<<"] "<<nTmDiff<<" s NO ACTIVE,KICK IT OUT.\n";
							// 踢出
							conn->shutdown();
							//conn->forceCloseWithDelay(1.0);  // > round trip of the whole Internet.
							conn->forceClose();
						}
					}
					// 时间有跳变
					else if (nTmDiff < 0)
					{
						node->UpdTm();
					}
					else
					{
						break;
					}
				}
				++it;
			}
			else
			{
				// 连接已失效
				connlist.erase(it++);
			}
		}
	}

private:
	void* m_pInvoker;
	char m_index;
	EventLoop* m_loop;
	TcpServer m_server;
	int m_nIdleConnTime;			// 空连接超时踢出时间
	size_t m_nHighWaterMark;	    // 高水位值
	
	pfunc_on_connection m_pfunc_on_connection;
	pfunc_on_readmsg_tcp m_pfunc_on_readmsg_tcp;
	pfunc_on_sendok m_pfunc_on_sendok;
	pfunc_on_senderr m_pfunc_on_senderr;
	pfunc_on_highwatermark m_pfunc_on_highwatermark;

	net_ctrl_params_ptr m_ptrCtrlParams;
};
//----------------------------------------------TcpServer End----------------------------------------------------


//----------------------------------------------HttpServer Begin-------------------------------------------------

//----------------------------------httprequest--------------------------------
HttpRequest::HttpRequest(const std::string& strReqIp, uint16_t uReqPort)
{
	p = reinterpret_cast<void*>(new HttpParser(HTTP_REQUEST));
	m_bHasRsp100Continue = false;
	m_bIncomplete = false;
	m_strReqIp	= strReqIp;
	m_uReqPort	= uReqPort;
}
HttpRequest::~HttpRequest()
{
	delete reinterpret_cast<HttpParser* >(p);
}
// 是否需要保持连接,即connection是keep-alive还是close
// 返回值:true:keep-alive false:close
bool HttpRequest::IsKeepAlive() const
{
	HttpParser* psr = reinterpret_cast<HttpParser* >(p);
	if (psr)
	{
		return psr->should_keep_alive();
	}
	return false;
}

//返回整个http请求的报文长度(报文头+报文体)
size_t	HttpRequest::GetHttpRequestTotalLen() const
{
	HttpParser* psr = reinterpret_cast<HttpParser* >(p);
	if (psr)
	{
		return psr->get_http_total_req_len();
	}
	return 0;

}

// 获取http body的数据
// 返回值:body的内容和长度
size_t HttpRequest::GetBodyData(std::string& strBody) const
{
	HttpParser* psr = reinterpret_cast<HttpParser* >(p);
	if (psr)
	{
		size_t len = psr->get_http_body_len();
		if (psr->is_chunk_body())
		{
			strBody.resize(len);
			psr->get_http_body_chunk_data(const_cast<char* >(strBody.data()));
		}
		else
		{
			char* data = NULL;len = 0;
			psr->get_http_body_data(&data, &len);
			strBody.assign(data, len);
		}
		return len;
	}
	return 0;
}

// 获取请求url
// 返回值:url的内容以及长度
// 说明:如:post请求,POST /sms/std/single_send HTTP/1.1 ,/sms/std/single_send就是URL
//	   如:get请求,
size_t HttpRequest::GetUrl(std::string& strUrl) const
{
	HttpParser* psr = reinterpret_cast<HttpParser* >(p);
	if (psr)
	{
		char* url = NULL;size_t len = 0;
		psr->get_http_url(&url, &len);
		strUrl.assign(url, len);
		return len;
	}
	return 0;
}
// 获取请求客户端的IP
// 返回值:请求客户端的IP
std::string HttpRequest::GetClientRequestIp() const
{
	return m_strReqIp;
}

// 获取请求客户端的端口
// 返回值:请求客户端的端口
uint16_t HttpRequest::GetClientRequestPort() const
{
	return m_uReqPort;
}

// 获取User-Agent
// 返回值:User-Agent的内容以及长度
size_t HttpRequest::GetUserAgent(std::string& strUserAgent) const
{
	HttpParser* psr = reinterpret_cast<HttpParser* >(p);
	if (psr)
	{
		char* pUserAgent = NULL;size_t len = 0;
		psr->get_http_user_agent(&pUserAgent, &len);
		strUserAgent.assign(pUserAgent, len);
		return len;
	}
	return 0;
}

// 获取content-type
// 返回值:CONTENT_TYPE_XML 代表 xml,CONTENT_TYPE_JSON 代表 json,CONTENT_TYPE_URLENCODE 代表 urlencode,CONTENT_TYPE_UNKNOWN 代表 未知类型
// 说明:暂只提供这三种
int HttpRequest::GetContentType() const
{
	HttpParser* psr = reinterpret_cast<HttpParser* >(p);
	if (psr)
	{
		return psr->get_http_content_type();
	}
	return CONTENT_TYPE_UNKNOWN;
}

// 获取host
// 返回值:host的内容以及长度
size_t HttpRequest::GetHost(std::string& strHost) const
{
	HttpParser* psr = reinterpret_cast<HttpParser* >(p);
	if (psr)
	{
		char* host = NULL;size_t len = 0;
		psr->get_http_host(&host, &len);
		strHost.assign(host, len);
		return len;
	}
	return 0;
}

// 获取x-forwarded-for
// 返回值:X-Forwarded-For的内容以及长度
size_t HttpRequest::GetXforwardedFor(std::string& strXforwardedFor) const
{
	HttpParser* psr = reinterpret_cast<HttpParser* >(p);
	if (psr)
	{
		char* x_forwarded_for = NULL;size_t len = 0;
		psr->get_http_x_forwarded_for(&x_forwarded_for, &len);
		strXforwardedFor.assign(x_forwarded_for, len);
		return len;
	}
	return 0;
}

// 获取http请求的类型
// 返回值:HTTP_REQUEST_POST 代表 post请求; HTTP_REQUEST_GET 代表 get请求 ; HTTP_REQUEST_UNKNOWN 代表 未知(解析包头有错误)
int HttpRequest::GetRequestType() const
{
	HttpParser* psr = reinterpret_cast<HttpParser* >(p);
	if (psr)
	{
		return psr->get_http_request_type();
	}
	return HTTP_REQUEST_UNKNOWN;
}

// 获取SOAPAction(仅当是soap时才有用)
// 返回值:SOAPAction内容以及长度
size_t HttpRequest::GetSoapAction(std::string& strSoapAction) const
{
	HttpParser* psr = reinterpret_cast<HttpParser* >(p);
	if (psr)
	{
		char* soapaction = NULL;size_t len = 0;
		psr->get_http_soapaction(&soapaction, &len);
		strSoapAction.assign(soapaction, len);
		return len;
	}
	return 0;
}

//获取Wap Via
//返回值:via内容以及长度
size_t	HttpRequest::GetVia(std::string& strVia) const
{
	HttpParser* psr = reinterpret_cast<HttpParser* >(p);
	if (psr)
	{
		char* via = NULL;size_t len = 0;
		psr->get_http_via(&via, &len);
		strVia.assign(via, len);
		return len;
	}
	return 0;
}

//获取x-wap-profile
//返回值:x-wap-profile内容以及长度
size_t	HttpRequest::GetXWapProfile(std::string& strXWapProfile) const
{
	HttpParser* psr = reinterpret_cast<HttpParser* >(p);
	if (psr)
	{
		char* xwapprofile = NULL;size_t len = 0;
		psr->get_http_x_wap_profile(&xwapprofile, &len);
		strXWapProfile.assign(xwapprofile, len);
		return len;
	}
	return 0;
}

//获取x-browser-type
//返回值:x-browser-type内容以及长度
size_t	HttpRequest::GetXBrowserType(std::string& strXBrowserType) const
{
	HttpParser* psr = reinterpret_cast<HttpParser* >(p);
	if (psr)
	{
		char* xbrowsertype = NULL;size_t len = 0;
		psr->get_http_x_browser_Type(&xbrowsertype, &len);
		strXBrowserType.assign(xbrowsertype, len);
		return len;
	}
	return 0;
}

//获取x-up-bear-type
//返回值:x-up-bear-type内容以及长度
size_t	HttpRequest::GetXUpBearType(std::string& strXUpBearType) const
{
	HttpParser* psr = reinterpret_cast<HttpParser* >(p);
	if (psr)
	{
		char* xupbearype = NULL;size_t len = 0;
		psr->get_http_x_up_bear_Type(&xupbearype, &len);
		strXUpBearType.assign(xupbearype, len);
		return len;
	}
	return 0;
}

//获取http头中任一个域对应的值(域不区分大小写)
//返回值:域对应的值
size_t	HttpRequest::GetHttpHeaderFieldValue(const char* field, std::string& strValue) const
{
	HttpParser* psr = reinterpret_cast<HttpParser* >(p);
	if (psr)
	{
		char* value = NULL;size_t len = 0;
		psr->get_http_header_field_value(field, &value, &len);
		strValue.assign(value, len);
		return len;
	}
	return 0;
}

//获取整个http报文的内容(头+体)
//返回值:http报文的指针,以及长度
size_t	HttpRequest::GetHttpRequestContent(std::string& strHttpRequestContent) const
{
	HttpParser* psr = reinterpret_cast<HttpParser* >(p);
	if (psr)
	{
		char* httprequestcontent = NULL;size_t len = 0;
		psr->get_http_request_content(&httprequestcontent, &len);
		strHttpRequestContent.assign(httprequestcontent, len);
		return len;
	}
	return 0;
}

// ..........................附带提供一组解析urlencode的函数......................
// 解析key,value格式的body,如:cmd=aa&seqid=1&msg=11
// 注意:仅当GetContentType返回CONTENT_TYPE_URLENCODE才可以使用该函数去解析
bool HttpRequest::ParseUrlencodedBody(const char* pbody/*http body 如:cmd=aa&seqid=1&msg=11*/, size_t len/*http body的长度*/) const
{
	HttpParser* psr = reinterpret_cast<HttpParser* >(p);
	if (psr)
	{
		return psr->parse_http_body(pbody, len);
	}
	return false;
}

// 通过key获取value的指针和长度,不做具体拷贝.若返回false代表key不存在
// 注意:必须先调用ParseUrlencodedBody后才可以使用该函数
bool HttpRequest::GetUrlencodedBodyValue(const char* key/*key,如:cmd=aa,cmd就是key,aa就是value*/, std::string& value) const
{
	HttpParser* psr = reinterpret_cast<HttpParser* >(p);
	if (psr)
	{
		return psr->get_value(key, value);
	}
	return false;
}
//................................................................................
//重置一些控制标志
void HttpRequest::ResetSomeCtrlFlag()
{
	m_bHasRsp100Continue = false;
	m_bIncomplete = false;
	m_strRequest = "";
}

//置已经回复过100-continue标志
void HttpRequest::SetHasRsp100Continue()
{
	m_bHasRsp100Continue = true;
}

//是否需要回复100continue;
bool HttpRequest::HasRsp100Continue()
{
	return m_bHasRsp100Continue;
}

//保存已接收到的数据
void HttpRequest::SaveIncompleteData(const char* pData, size_t nLen)
{
	m_strRequest.append(pData, nLen);
}

//重置解析器
void HttpRequest::ResetParser()
{
	HttpParser* psr = reinterpret_cast<HttpParser* >(p);
	if (psr)
	{
		psr->reset_parser();
	}
}

//报文头中是否有100-continue;
bool HttpRequest::b100ContinueInHeader()
{
	bool bRet = false;

	HttpParser* psr = reinterpret_cast<HttpParser* >(p);
	if (psr)
	{
		bRet = psr->expect_100_continue();
	}

	return bRet;
}

// 解析http请求
// 返回值 0:成功 此时要看over是否为true,若为true代表解析完成,若为false代表包未收全
// 返回值 1:代表解析失败 需关闭连接
int	HttpRequest::ParseHttpRequest(const char* data, size_t len, size_t maxlen, bool& over)
{
	int nRet = 1;

	HttpParser* parser = reinterpret_cast<HttpParser* >(p);
	if (parser)
	{
		//限制错误的长度
		if (len <= 0 || len > maxlen) 
		{
			return nRet = 1;
		}

		//如果数据刚来开头就是"\r","\n"直接拒绝,如果是残包,则继续接收
		if ((data[0] == '\r' || data[0] == '\n') && !m_bIncomplete)
		{
			return nRet = 1;
		}
		
		// 如果是未收完整的包,先附加上到上次保存的部分,然后再使用存储的部分进行解析
		if (m_bIncomplete) SaveIncompleteData(data, len);
		nRet = parser->execute_http_parse(m_bIncomplete?m_strRequest.c_str():data, m_bIncomplete?m_strRequest.size():len, over);
		// 包没收完，继续收，不用回调, 但必须保存起来, 并且置标志该包需使用上次保存起来的数据进行下次解析
		if (!m_bIncomplete && 0 == nRet && !over)
		{
			SaveIncompleteData(data, len);
			m_bIncomplete = true;
		}

		// 若接收了maxlen个长度仍然不是一个完整包,此时关闭连接
		if (!over && m_strRequest.size() >= maxlen)
		{
			nRet = 1;
		}
	}

	return nRet;
}

//-----------------------------------------------------------------------------
class StatusCodeReason
{
    public:
        StatusCodeReason()
        {
        	unknown_reason = "Unknown Error";

            m_reasons.resize(600);
            for (size_t i = 0; i < m_reasons.size(); ++i)
            {
            	m_reasons[i] = unknown_reason;
            }
            m_reasons[100] = "Continue";
            m_reasons[101] = "Switching Protocols";
            m_reasons[200] = "OK";
            m_reasons[201] = "Created";
            m_reasons[202] = "Accepted";
            m_reasons[203] = "Non-Authoritative Information";
            m_reasons[204] = "No Content";
            m_reasons[205] = "Reset Content";
            m_reasons[206] = "Partial Content";
            m_reasons[300] = "Multiple Choices";
            m_reasons[301] = "Moved Permanently";
            m_reasons[302] = "Found";
            m_reasons[303] = "See Other";
            m_reasons[304] = "Not Modified";
            m_reasons[305] = "Use Proxy";
            m_reasons[307] = "Temporary Redirect";
            m_reasons[400] = "Bad Request";
            m_reasons[401] = "Unauthorized";
            m_reasons[402] = "Payment Required";
            m_reasons[403] = "Forbidden";
            m_reasons[404] = "Not Found";
            m_reasons[405] = "Method Not Allowed";
            m_reasons[406] = "Not Acceptable";
            m_reasons[407] = "Proxy Authentication Required";
            m_reasons[408] = "Request Time-out";
            m_reasons[409] = "Conflict";
            m_reasons[410] = "Gone";
            m_reasons[411] = "Length Required";
            m_reasons[412] = "Precondition Failed";
            m_reasons[413] = "Request Entity Too Large";
            m_reasons[414] = "Request-URI Too Large";
            m_reasons[415] = "Unsupported Media Type";
            m_reasons[416] = "Requested range not satisfiable";
            m_reasons[417] = "Expectation Failed";
            m_reasons[500] = "Internal Server Error";
            m_reasons[501] = "Not Implemented";
            m_reasons[502] = "Bad Gateway";
            m_reasons[503] = "Service Unavailable";
            m_reasons[504] = "Gateway Time-out";
            m_reasons[505] = "HTTP Version not supported";
        }

        const std::string& GetStatusCodeReason(int code) const
        {
            if(code >= static_cast<int>(m_reasons.size()))
            {
                return unknown_reason;
            }

            return m_reasons[code].empty() ? unknown_reason : m_reasons[code];
        }

    private:
        std::vector<std::string> m_reasons;

		std::string unknown_reason;
};

StatusCodeReason g_statuscoderesson;

//----------------------------------httpresponse--------------------------------
HttpResponse::HttpResponse()
{
	m_strResponse = "";
	m_bKeepAlive  = false;
	m_nStatusCode = 200;
	m_strContentType = "application/x-www-form-urlencoded; charset=UTF-8";
	m_bHaveSetStatusCode 	= false;
	m_bHaveSetContentType	= false;
	m_bHaveSetKeepAlive  	= false;
	m_bHaveSetResponseBody 	= false;
    m_bHaveSetLocation      = false;
    m_strLocation           = "";
	m_strOtherFieldValue	= "";
}

HttpResponse::~HttpResponse()
{
}

// 设置状态码，200，400，404 etc.
// 注意:该函数内部有校验，只允许调用一次
void HttpResponse::SetStatusCode(int nStatusCode)
{
	if (!m_bHaveSetStatusCode)
	{
		m_nStatusCode = nStatusCode;
		if (200 != m_nStatusCode && 202 != m_nStatusCode && 100 != m_nStatusCode)
		{
			m_bKeepAlive = false;// 非成功错误码时，强制将连接置为不允许保持
			FormatHttpResponse(g_statuscoderesson.GetStatusCodeReason(m_nStatusCode));
		}
	}
	m_bHaveSetStatusCode = true;
}

// 设置conetnettype，如：text/json，application/x-www-form-urlencoded，text/xml
// 注意:该函数内部有校验，只允许调用一次
void HttpResponse::SetContentType(const std::string& strContentType)
{
	if (!m_bHaveSetContentType)
	{
		m_strContentType = strContentType;
	}
	m_bHaveSetContentType = true;
}


//设置Location，如：当状态码为301、302进行协议跳转时，需要设置该参数指定跳转地址
//注意:该函数内部有校验，只允许调用一次
void HttpResponse::SetLocation(const std::string& strLocation)
{
	if (!m_bHaveSetLocation)
	{
		m_strLocation = strLocation;
	}
	m_bHaveSetLocation = true;
}

//设置一些没有明确定义的非标准的http回应头
//格式如:x-token: AAA\r\nx-auth: BBB\r\n
void HttpResponse::SetHeader(const std::string& strFieldAndValue)
{
	m_strOtherFieldValue = strFieldAndValue;
}


// 设置连接是否保持，bKeepAlive=1保持:Keep-Alive，bKeepAlive=0不保持:Close
// 注意:该函数内部有校验，只允许调用一次
void HttpResponse::SetKeepAlive(bool bKeepAlive)
{
	if (!m_bHaveSetKeepAlive)
	{
		m_bKeepAlive = bKeepAlive;
	}
	m_bHaveSetKeepAlive = true;
}
// 是否需要保持长连接
bool HttpResponse::IsKeepAlive() const
{
	return m_bKeepAlive;
}

// 设置回应包的包体
// 注意:必须在SetStatusCode&SetContentType&SetKeepAlive都设置OK了以后才可以调用
// 注意:该函数内部有校验，只允许调用一次
void HttpResponse::SetResponseBody(const std::string& strRspBody)
{
	if (!m_bHaveSetResponseBody)
	{
		FormatHttpResponse(strRspBody);
	}
	m_bHaveSetResponseBody = true;
}
void HttpResponse::FormatHttpResponse(const std::string& strRspBody)
{
	if (200 == m_nStatusCode || 202 == m_nStatusCode)
	{
		stdstring_format(m_strResponse,
						"HTTP/1.1 %03d %s\r\n"
						"Content-Type: %s\r\n"
						"Connection: %s\r\n"
						"%s"
						"Content-Length: %d\r\n\r\n",
						m_nStatusCode,
						g_statuscoderesson.GetStatusCodeReason(m_nStatusCode).c_str(),
						m_strContentType.c_str(),
						m_bKeepAlive?"Keep-Alive":"Close",
						m_strOtherFieldValue.c_str(),
						strRspBody.size()
						);
		m_strResponse.append(strRspBody.c_str(), strRspBody.size());
	}
	else if (100 == m_nStatusCode)
	{
		m_strResponse = "100-continue";
	}
    else if (301 == m_nStatusCode || 302 == m_nStatusCode)
    {
		stdstring_format(m_strResponse,
						"HTTP/1.1 %03d %s\r\n"
						"Location: %s\r\n"
						"Content-Type: %s\r\n"
						"Connection: %s\r\n"
						"%s"
						"Content-Length: %d\r\n\r\n",
						m_nStatusCode,
						g_statuscoderesson.GetStatusCodeReason(m_nStatusCode).c_str(),
						m_strLocation.c_str(),
						m_strContentType.c_str(),
						m_bKeepAlive?"Keep-Alive":"Close",
						m_strOtherFieldValue.c_str(),
						strRspBody.size()
						);

        m_strResponse.append(strRspBody.c_str(), strRspBody.size());
    }
	else
	{
		// 如果有body就直接格式化，如果body没填就填标准的statuscodereason
		const char* pStatusCodeReason = g_statuscoderesson.GetStatusCodeReason(m_nStatusCode).c_str();
		const char* pBody = strRspBody.c_str();
		if (strRspBody.length() <= 0)
		{
			pBody = pStatusCodeReason;
		}

		stdstring_format(m_strResponse,
						"HTTP/1.1 %03d %s\r\n"
						"Content-Type: text/plain\r\n"
						"Connection: Close\r\n"
						"%s"
						"Content-Length: %d\r\n\r\n"
						"%03d %s",
						m_nStatusCode,
						pStatusCodeReason,
						m_strOtherFieldValue.c_str(),
						strRspBody.size()+3+1,
						m_nStatusCode,
						pBody
						);

	}
}

// 将上层代码格式化好的完整的回应报文（头+体）设置进来
// 使用该函数时SetStatusCode&SetContentType&SetKeepAlive仍可以同时设置，但SetResponseBody一定不能调用
void HttpResponse::SetHttpResponse(std::string& strResponse)
{
	m_strResponse.swap(strResponse);
}

// 返回格式化好的httpresponse字符串-注意:必须在SetResponseBody设置OK了以后才可以调用
const std::string& HttpResponse::GetHttpResponse() const
{
	return m_strResponse;
}

// 返回http回应的总长度(头+体)-注意:必须在SetResponseBody设置OK了以后才可以调用
size_t HttpResponse::GetHttpResponseTotalLen() const
{
	return m_strResponse.size();
}

//-----------------------------------------------------------------------------

class HttpNetServerImpl
{
public:
	HttpNetServerImpl(void* pInvoker,char index,EventLoop* loop,const InetAddress& listenAddr,int threadnum,int idleconntime,	bool bReusePort,
		pfunc_on_connection pOnConnection, pfunc_on_readmsg_http pOnReadMsg, pfunc_on_sendok pOnSendOk, 
		pfunc_on_senderr pOnSendErr, pfunc_on_highwatermark pOnHighWaterMark,
		size_t nDefRecvBuf, size_t nMaxRecvBuf, size_t nMaxSendQue, net_ctrl_params_ptr ptrCtrlParams)
	: 	m_pInvoker(pInvoker),
		m_index(index),
		m_loop(loop),
		m_server(loop, stdstring_format("HttpSvr%02d_%02d", index + 1, loop->getidinpool() + 1).c_str(), listenAddr, bReusePort ? TcpServer::kReusePort : TcpServer::kNoReusePort, nDefRecvBuf, nMaxRecvBuf, nMaxSendQue),
		m_nIdleConnTime(idleconntime),
		m_nHighWaterMark(nMaxSendQue),
		m_pfunc_on_connection(pOnConnection),
		m_pfunc_on_readmsg_http(pOnReadMsg),
		m_pfunc_on_sendok(pOnSendOk),
		m_pfunc_on_senderr(pOnSendErr),
		m_pfunc_on_highwatermark(pOnHighWaterMark),
		m_ptrCtrlParams(ptrCtrlParams)

	{
		// 绑定连接事件回调(connect,disconnect)
		m_server.setConnectionCallback(std::bind(&HttpNetServerImpl::onConnection, this, _1));
		// 绑定收到的数据回调
		m_server.setMessageCallback(std::bind(&HttpNetServerImpl::onMessage, this, _1, _2, _3));
		// 绑定写数据完成回调
		m_server.setWriteCompleteCallback(std::bind(&HttpNetServerImpl::onWriteOk, this, _1, _2));
		// 绑定写数据异常或失败回调
		m_server.setWriteErrCallback(std::bind(&HttpNetServerImpl::onWriteErr, this, _1, _2, _3));
		// 绑定线程启动成功回调
		m_server.setThreadInitCallback(std::bind(&HttpNetServerImpl::onThreadInit, this, _1));
		// 设置工作线程数
		m_server.setThreadNum(threadnum);
		// 启动server
		m_loop->runInLoop(std::bind(&HttpNetServerImpl::start, this));
	}

	// 重置空连接踢空时间
	void ResetIdleConnTime(int idleconntime)
	{
		m_nIdleConnTime = idleconntime;
	}

  	// 回应
	int SendHttpResponse(const TcpConnectionPtr& conn, const boost::any& params, const HttpResponse& rsp, int timeout)
	{
		if (!rsp.IsKeepAlive()) conn->setNeedCloseFlag();
		
		if (m_ptrCtrlParams->g_bEnableSendLog_http) LOG_INFO << "[HTTP][SEND][" << conn->getConnuuid() << "]\n" << rsp.GetHttpResponse().c_str();

		return conn->send(rsp.GetHttpResponse(), params, timeout);
	}

	// 强制关闭连接
	void ForceCloseConnection(const TcpConnectionPtr& conn)
	{
		conn->shutdown();
		//conn->forceCloseWithDelay(1.0);  // > round trip of the whole Internet.
		conn->forceClose();
	}

	// 设置http会话的信息(请传session类的智能指针，并且只允许调用一次&session类中自行控制多线程互斥问题)
	void SetHttpSessionInfo(const TcpConnectionPtr& conn, const boost::any& sessioninfo)
	{
		conn->setSessionData(sessioninfo);
	}

	// 设置高水位值
	void SetHttpHighWaterMark(const TcpConnectionPtr& conn, size_t highWaterMark)
	{
		conn->setHighWaterMark(highWaterMark);
	}

	void StopHttpServer()
	{
		m_server.stop();
	}
private:
 	void AddToConnList(const TcpConnectionPtr& conn)
 	{
 		conn->setTcpNoDelay(true);

 		ConnNodePtr node(new ConnNode(m_index));
		node->UpdTm();
		WeakConnectionList& connlist = ThreadLocalSingleton<WeakConnectionList>::instance();
		connlist.push_back(conn);
		node->UpdPos(--connlist.end());
    	conn->setContext(node);

		if (m_ptrCtrlParams->g_unique_node_id >= 1 && m_ptrCtrlParams->g_unique_node_id <= 8000)
		{
			uint64_t conn_uuid = MakeConnuuid(m_ptrCtrlParams->g_unique_node_id, m_ptrCtrlParams->g_nSequnceId);
			conn->setConnuuid(conn_uuid);
			WeakTcpConnectionPtr weakconn(conn);
			MutexLockGuard lock(m_ptrCtrlParams->g_mapHttpConns_lock);
			m_ptrCtrlParams->g_mapHttpConns.insert(std::make_pair(conn_uuid, weakconn));
		}
 	}

	void UpdateConnList(const TcpConnectionPtr& conn, bool bDel)
 	{
 		if (conn && !conn->getContext().empty())
 		{
	 		ConnNodePtr node(boost::any_cast<ConnNodePtr>(conn->getContext()));
			if (node)
			{
				WeakConnectionList& connlist = ThreadLocalSingleton<WeakConnectionList>::instance();
				if (!bDel)
				{
					node->UpdTm();
					connlist.splice(connlist.end(), connlist, node->GetPos());
					node->UpdPos(--connlist.end());
				}
				else
				{
					if (m_ptrCtrlParams->g_unique_node_id >= 1 && m_ptrCtrlParams->g_unique_node_id <= 8000)
					{
						MutexLockGuard lock(m_ptrCtrlParams->g_mapHttpConns_lock);
						m_ptrCtrlParams->g_mapHttpConns.erase(conn->getConnuuid());
					}

					WeakConnectionList::iterator it = node->GetPos();
	    			if (!connlist.empty() && it != connlist.end())
					{
						connlist.erase(it);
						//置为无效pos
						node->UpdPos(connlist.end());
	    			}
				}
			}
 		}
 	}
private:
	void start()
	{
  		m_server.start();
	}
	
	void onHighWater(const TcpConnectionPtr& conn, size_t highWaterMark)
	{
		if (!conn) return;

		int nRet = 0;
		if (m_pfunc_on_highwatermark)
		{
			WeakTcpConnectionPtr weakconn(conn);
			nRet = m_pfunc_on_highwatermark(m_pInvoker, conn->getConnuuid(), weakconn, conn->getSessionData(), highWaterMark);
		}

		if (nRet < 0 || conn->bNeedClose())
		{
			conn->shutdown();
			//conn->forceCloseWithDelay(1.0);  // > round trip of the whole Internet.
			conn->forceClose();
		}
	}

	void onConnection(const TcpConnectionPtr& conn)
	{
		if (!conn) return;

		if (conn->connected())
		{
			conn->setHighWaterMarkCallback(std::bind(&HttpNetServerImpl::onHighWater, this, _1, _2), m_nHighWaterMark);
			
			AddToConnList(conn);

			// 限制最大连接数
			if (m_ptrCtrlParams->g_nCurrConnNum_Http.incrementAndGet() > m_ptrCtrlParams->g_nMaxConnNum_Http)
		    {
		      	conn->shutdown();
		      	//conn->forceCloseWithDelay(1.0);  // > round trip of the whole Internet.
		      	conn->forceClose();
				LOG_WARN << "HttpServer reached the maxconnnum:" << m_ptrCtrlParams->g_nMaxConnNum_Http;
				return;
		    }

			// 申请一个httprequest对象，保存到连接当中用于做http请求解析
			HttpRequestPtr reqptr(new HttpRequest(conn->peerAddress().toIp(), conn->peerAddress().toPort()));
			conn->setExInfo(reqptr);

			if (m_pfunc_on_connection)
			{
				WeakTcpConnectionPtr weakconn(conn);
				int nRet = m_pfunc_on_connection(m_pInvoker, conn->getConnuuid(), weakconn, conn->getSessionData(), true, conn->peerAddress().toIpPort().c_str());
				if (nRet < 0)
				{
					conn->shutdown();
					//conn->forceCloseWithDelay(1.0);  // > round trip of the whole Internet.
					conn->forceClose();
				}
				else if (0 == nRet)
				{
					// 回调成功,无需处理
				}
				else
				{
					// 其他返回值暂无定义,暂不处理
				}
			}
		}
		else if (conn->disconnected())
		{
			m_ptrCtrlParams->g_nCurrConnNum_Http.decrement();
			//printf("%d-link gone:%s\n", m_nCurrConnNum.get(),conn->peerAddress().toIpPort().c_str());

			if (m_pfunc_on_connection)
			{
				WeakTcpConnectionPtr weakconn(conn);
				m_pfunc_on_connection(m_pInvoker, conn->getConnuuid(), weakconn, conn->getSessionData(), false, conn->peerAddress().toIpPort().c_str());
			}

			UpdateConnList(conn, true);
		}
	}

	void DealHttpRequest(const TcpConnectionPtr& conn, Buffer* buf, Timestamp time)
	{
		if (conn && !conn->getExInfo().empty())
		{
			HttpRequestPtr reqptr = boost::any_cast<HttpRequestPtr>(conn->getExInfo());
			// Buffer to StringPiece
			StringPiece strRecvMsg = buf->toStringPiece();
			// write logs
			if (m_ptrCtrlParams->g_bEnableRecvLog_http) LOG_INFO << "[HTTP][RECV][" << conn->getConnuuid() << "][" << conn->peerAddress().toIpPort() << "]\n" << strRecvMsg.data();
			bool bParseOver = false;
			// 执行解析
			int nParseRet = reqptr->ParseHttpRequest(strRecvMsg.data(), strRecvMsg.size(), m_ptrCtrlParams->g_nMaxRecvBuf_Http, bParseOver);
			// 解析析成功
			if (0 == nParseRet)
			{
				// 包已收完，并解析完，回调给上层
				if (bParseOver)
				{
					int nReqType = reqptr->GetRequestType();				
					if (m_pfunc_on_readmsg_http && (nReqType == HTTP_REQUEST_POST || nReqType == HTTP_REQUEST_GET)) //discard unknown request
					{
						WeakTcpConnectionPtr weakconn(conn);
						int nRet = m_pfunc_on_readmsg_http(m_pInvoker, conn->getConnuuid(), weakconn, conn->getSessionData(), reqptr.get());
						if (nRet < 0)
						{
							conn->shutdown();
							//conn->forceCloseWithDelay(1.0);  // > round trip of the whole Internet.
							conn->forceClose();
						}
						else if (0 == nRet)
						{
							// 回调成功,无需处理
						}
						else
						{
							// 其他返回值暂无定义,暂不处理
						}
					}

					// 将buf中的指针归位
					buf->retrieveAll();

					// 复位解析器
					reqptr->ResetParser();

					// 控制标志复位
					reqptr->ResetSomeCtrlFlag();
				}
				else
				{
					// 解析到Expect: 100-Continue
					// 并且没有回过,则响应100-Continue
					if (!reqptr->HasRsp100Continue() && reqptr->b100ContinueInHeader())
					{
						//置已回应过100continue标志
						reqptr->SetHasRsp100Continue();

						// 回应100-continue
						boost::any params;
						conn->send("100-continue", params);

						// 记录日志
						if (m_ptrCtrlParams->g_bEnableSendLog_http) LOG_INFO << "[HTTP][SEND][" << conn->getConnuuid() << "][" << conn->peerAddress().toIpPort() << "]\n100-continue";
					}

					// 将buf中的指针归位
					buf->retrieveAll();

					// 复位解析器
					reqptr->ResetParser();
				}
			}
			else
			{
				// 将buf中的指针移归位
				buf->retrieveAll();

				// 控制标志复位
				reqptr->ResetSomeCtrlFlag();

				// 复位解析器
				reqptr->ResetParser();

				// 设置连接需要关闭标志
				conn->setNeedCloseFlag();

				// 解析失败，直接生成失败的response,回:400
				boost::any params;
				conn->send("HTTP/1.1 400 Bad Request\r\n\r\n", params);

				// 记录日志
				LOG_WARN<<"[HTTP][SEND]["<<conn->getConnuuid()<<"]["<<conn->peerAddress().toIpPort()<<"]\nillegal data size or illegal data format\nHTTP/1.1 400 Bad Request\r\n\r\n";
			}
		}
	}

	void onMessage(const TcpConnectionPtr& conn, Buffer* buf, Timestamp time)
	{
		if (!conn) return;

		m_ptrCtrlParams->g_nTotalReqNum_Http.increment();

		UpdateConnList(conn, false);

		DealHttpRequest(conn, buf, time);
	}

	void onWriteOk(const TcpConnectionPtr& conn, const boost::any& params)
	{
		if (!conn) return;

		// 减少总待发出计数
		m_ptrCtrlParams->g_nTotalWaitSendNum_http.decrement();
		
		int nRet = 0;
		if (m_pfunc_on_sendok)
		{
			WeakTcpConnectionPtr weakconn(conn);
			nRet = m_pfunc_on_sendok(m_pInvoker, conn->getConnuuid(), weakconn, conn->getSessionData(), params);
		}

		if (nRet < 0 || conn->bNeedClose())
		{
			conn->shutdown();
			//conn->forceCloseWithDelay(1.0);  // > round trip of the whole Internet.
			conn->forceClose();
		}
	}

	void onWriteErr(const TcpConnectionPtr& conn, const boost::any& params, int errCode)
	{
		if (!conn) return;

		// 减少总待发出计数
		m_ptrCtrlParams->g_nTotalWaitSendNum_http.decrement();

		int nRet = 0;
		if (m_pfunc_on_senderr)
		{
			WeakTcpConnectionPtr weakconn(conn);
			nRet = m_pfunc_on_senderr(m_pInvoker, conn->getConnuuid(), weakconn, conn->getSessionData(), params, errCode);
		}

		if (nRet < 0 || conn->bNeedClose())
		{
			conn->shutdown();
			//conn->forceCloseWithDelay(1.0);  // > round trip of the whole Internet.
			conn->forceClose();
		}
	}

	void onThreadInit(EventLoop* loop)
	{
		m_server.getLoop()->runInLoop(std::bind(&HttpNetServerImpl::startTimer, this));
	}

	// 开启定时器
	void startTimer()
	{
		EventLoop* loop = m_server.threadPool()->getNextLoop();
		loop->runInLoop(std::bind(&HttpNetServerImpl::InitConnList, this));
		loop->runEvery(1.0, std::bind(&HttpNetServerImpl::onCheckConnTimeOut, this));
	}

    // 初始化连接队列
	void InitConnList()
	{
		ThreadLocalSingleton<WeakConnectionList>::instance();
	}

	// 定时器,定时检测超时
	void onCheckConnTimeOut()
	{
		WeakConnectionList& connlist = ThreadLocalSingleton<WeakConnectionList>::instance();
		for (WeakConnectionList::iterator it = connlist.begin(); it != connlist.end();)
		{
			TcpConnectionPtr conn(it->lock());
			if (conn && !conn->getContext().empty())
			{
				ConnNodePtr node(boost::any_cast<ConnNodePtr>(conn->getContext()));
				if (node)
				{
					int nTmDiff = node->TmDiff();
					// 超时
					if (nTmDiff > m_nIdleConnTime)
					{
						if (conn->connected())
						{
							LOG_INFO<<"IP:["<<conn->peerAddress().toIpPort()<<"] CONNUUID:["<<conn->getConnuuid()<<"] "<<nTmDiff<<" s NO ACTIVE,KICK IT OUT.\n";
							// 踢出
							conn->shutdown();
							//conn->forceCloseWithDelay(1.0);  // > round trip of the whole Internet.
							conn->forceClose();
						}
					}
					// 时间有跳变
					else if (nTmDiff < 0)
					{
						node->UpdTm();
					}
					else
					{
						break;
					}
				}
				++it;
			}
			else
			{
				// 连接已失效
				connlist.erase(it++);
			}
		}
	}

private:
	void* m_pInvoker;
	char m_index;
	EventLoop* m_loop;
	TcpServer m_server;
	int m_nIdleConnTime;		// 空连接超时踢出时间
	size_t m_nHighWaterMark;	// 高水位值
	
	pfunc_on_connection m_pfunc_on_connection;
	pfunc_on_readmsg_http m_pfunc_on_readmsg_http;
	pfunc_on_sendok m_pfunc_on_sendok;
	pfunc_on_senderr m_pfunc_on_senderr;
	pfunc_on_highwatermark m_pfunc_on_highwatermark;

	net_ctrl_params_ptr m_ptrCtrlParams;
};
//----------------------------------------------HttpServer End----------------------------------------------------


//----------------------------------------------NetServer Ctrl---------------------------------------------------
class NetServerCtrlInfo
{
public:
	NetServerCtrlInfo():
		m_ptrCtrlParams(new net_ctrl_params())
	{
		m_ptrCtrlParams->g_bExit_tcp  = false;
		m_ptrCtrlParams->g_bExit_http = false;
		m_pTcpLoop = NULL;
		m_pHttpLoop = NULL;
		m_pHttpEventLoopThreadPool = NULL;
	}
	~NetServerCtrlInfo()
	{
	}
public:
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// TCP服务
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// 启动TCP服务
	void StartTcpServer(void* pInvoker,uint16_t listenport,int threadnum,int idleconntime,int maxconnnum,bool bReusePort,
		pfunc_on_connection pOnConnection, pfunc_on_readmsg_tcp pOnReadMsg, pfunc_on_sendok pOnSendOk, 
		pfunc_on_senderr pOnSendErr, pfunc_on_highwatermark pOnHighWaterMark,
		size_t nDefRecvBuf, size_t nMaxRecvBuf, size_t nMaxSendQue)
	{
		LOG_INFO << "TcpServer Start";
		
		threadnum<=0?threadnum=0:1;
		threadnum>128?threadnum=128:1;

		m_ptrCtrlParams->g_nMaxRecvBuf_Tcp = nMaxRecvBuf;
		m_ptrCtrlParams->g_nMaxConnNum_Tcp = maxconnnum;

		m_ptrCtrlParams->g_bExit_tcp = false;

		m_pTcpLoop = new EventLoop();
		InetAddress listenAddr(listenport, false, false);
		//SO_REUSEPORT
		if (bReusePort)
		{
			m_pTcpEventLoopThreadPool = new EventLoopThreadPool(m_pTcpLoop, "TcpLoop");
			//int nLoopNum = threadnum;
			int nLoopNum = threadnum/2;
			nLoopNum<=0?nLoopNum=1:1;
			nLoopNum>8?nLoopNum=8:1;
			m_pTcpEventLoopThreadPool->setThreadNum(nLoopNum);
			m_pTcpEventLoopThreadPool->start();

			for (int i = 0; i < threadnum; ++i)
			{
				m_TcpNetServerImpls.push_back(new TcpNetServerImpl(pInvoker, static_cast<char>(i & 0xff), m_pTcpEventLoopThreadPool->getNextLoop(), 
					listenAddr, 1, idleconntime, bReusePort, pOnConnection, pOnReadMsg, pOnSendOk, pOnSendErr, pOnHighWaterMark, nDefRecvBuf, nMaxRecvBuf, nMaxSendQue, m_ptrCtrlParams));
			}
		}
		else
		{
			m_TcpNetServerImpls.push_back(new TcpNetServerImpl(pInvoker, 0, m_pTcpLoop, listenAddr, threadnum, idleconntime, 
				bReusePort, pOnConnection, pOnReadMsg, pOnSendOk, pOnSendErr, pOnHighWaterMark, nDefRecvBuf, nMaxRecvBuf, nMaxSendQue, m_ptrCtrlParams));
		}
		//m_pTcpLoop->runEvery(1.0, std::bind(&NetServerCtrlInfo::onIntervalScanf_Tcp, this));
		m_pTcpLoop->loop();
		delete m_pTcpLoop;
		m_pTcpLoop = NULL;
		LOG_INFO << "TcpServer Quit";
	}

	void StopTcpServerInLoop(const std::shared_ptr<CountDownLatch>& latch)
	{	
		std::vector<TcpNetServerImpl*>::iterator it = m_TcpNetServerImpls.begin();
		for (; it != m_TcpNetServerImpls.end(); ++it)
		{
			(*it)->StopTcpServer();
		}

		m_TcpNetServerImpls.clear();

		delete m_pTcpEventLoopThreadPool;
		m_pTcpEventLoopThreadPool = NULL;
		latch->countDown();
	}
		
	// 关闭TCP服务
	void StopTcpServer()
	{
		net_ctrl_params_ptr pCtrlParams = m_ptrCtrlParams;
		if (pCtrlParams && !pCtrlParams->g_bExit_tcp && m_pTcpLoop != NULL)
		{
			std::shared_ptr<CountDownLatch> latch(new CountDownLatch(1));
			pCtrlParams->g_bExit_tcp = true;
			m_pTcpLoop->runInLoop(std::bind(&NetServerCtrlInfo::StopTcpServerInLoop, this, latch));
			latch->wait();	
			m_pTcpLoop->quit();
		}
	}
	
	// 获取当前TCP连接总数
	int GetCurrTcpConnNum()
	{
		net_ctrl_params_ptr pCtrlParams = m_ptrCtrlParams;
		if (pCtrlParams) return pCtrlParams->g_nCurrConnNum_Tcp.get();
		return 0;
	}
	// 获取当前TCP总请求数
	int64_t GetTotalTcpReqNum()
	{
		net_ctrl_params_ptr pCtrlParams = m_ptrCtrlParams;
		if (pCtrlParams) return pCtrlParams->g_nTotalReqNum_Tcp.get();
		return 0;
	}
	
	//获取当前TCP总的等待发送的数据包数量
	int64_t GetTotalTcpWaitSendNum()
	{
		net_ctrl_params_ptr pCtrlParams = m_ptrCtrlParams;
		if (pCtrlParams) return pCtrlParams->g_nTotalWaitSendNum_tcp.get();
		return 0;
	}
	// 发送信息
	int SendMsgWithTcpServer(uint64_t conn_uuid, const boost::any& conn, const boost::any& params, const char* szMsg, size_t nMsgLen, int timeout, bool bKeepAlive)
	{
		int ret = 1;
		net_ctrl_params_ptr pCtrlParams = m_ptrCtrlParams;
		if (pCtrlParams && !pCtrlParams->g_bExit_tcp) 
		{
			WeakTcpConnectionPtr weakconn;
			if (pCtrlParams->g_unique_node_id >= 1 && pCtrlParams->g_unique_node_id <= 8000)
			{
				MutexLockGuard lock(pCtrlParams->g_mapTcpConns_lock);
				std::map<uint64_t, WeakTcpConnectionPtr>::iterator it = pCtrlParams->g_mapTcpConns.find(conn_uuid);
				if (it != pCtrlParams->g_mapTcpConns.end())
				{
					weakconn = it->second;
				}
			}
			else
			{
				if (!conn.empty()) weakconn = boost::any_cast<WeakTcpConnectionPtr>(conn);
			}
			TcpConnectionPtr pConn(weakconn.lock());
			if (pConn && !pConn->getContext().empty())
			{
				ConnNodePtr node(boost::any_cast<ConnNodePtr>(pConn->getContext()));
				if (node)
				{
					ret = m_TcpNetServerImpls[node->GetIndex()]->SendMsgWithTcpServer(pConn, params, szMsg, nMsgLen, timeout, bKeepAlive);
				}
			}
			// 增加待发出总的计数
			if (0 == ret) pCtrlParams->g_nTotalWaitSendNum_tcp.increment();
		}
		return ret;
	}
	// 强制关闭TCP连接
	void ForceCloseTcpConnection(uint64_t conn_uuid, const boost::any& conn)
	{
		net_ctrl_params_ptr pCtrlParams = m_ptrCtrlParams;
		if (pCtrlParams && !pCtrlParams->g_bExit_tcp) 
		{
			WeakTcpConnectionPtr weakconn;
			if (pCtrlParams->g_unique_node_id >= 1 && pCtrlParams->g_unique_node_id <= 8000)
			{
				MutexLockGuard lock(pCtrlParams->g_mapTcpConns_lock);
				std::map<uint64_t, WeakTcpConnectionPtr>::iterator it = pCtrlParams->g_mapTcpConns.find(conn_uuid);
				if (it != pCtrlParams->g_mapTcpConns.end())
				{
					weakconn = it->second;
				}
			}
			else
			{
				if (!conn.empty()) weakconn = boost::any_cast<WeakTcpConnectionPtr>(conn);
			}
			TcpConnectionPtr pConn(weakconn.lock());
			if (pConn && !pConn->getContext().empty())
			{
				ConnNodePtr node(boost::any_cast<ConnNodePtr>(pConn->getContext()));
				if (node)
				{
					m_TcpNetServerImpls[node->GetIndex()]->ForceCloseConnection(pConn);
				}
			}
		}
	}
	// 重置TCP空连接踢空时间
	void ResetTcpIdleConnTime(int idleconntime)
	{
		net_ctrl_params_ptr pCtrlParams = m_ptrCtrlParams;
		if (pCtrlParams && !pCtrlParams->g_bExit_tcp) 
		{
			std::vector<TcpNetServerImpl*>::iterator itTcp = m_TcpNetServerImpls.begin();
			for (; itTcp != m_TcpNetServerImpls.end(); ++itTcp)
			{
				(*itTcp)->ResetIdleConnTime(idleconntime);
			}
		}
	}
	// 重置TCP可支持的最大连接数
	void ResetTcpMaxConnNum(int maxconnnum)
	{	
		net_ctrl_params_ptr pCtrlParams = m_ptrCtrlParams;
		if (pCtrlParams) pCtrlParams->g_nMaxConnNum_Tcp = maxconnnum;
	}

	// 设置tcp会话的信息(请传session类的智能指针，并且只允许调用一次&session类中自行控制多线程互斥问题)
	void SetTcpSessionInfo(uint64_t conn_uuid, const boost::any& conn, const boost::any& sessioninfo)
	{
		net_ctrl_params_ptr pCtrlParams = m_ptrCtrlParams;
		if (pCtrlParams && !pCtrlParams->g_bExit_tcp) 
		{
			WeakTcpConnectionPtr weakconn;
			if (pCtrlParams->g_unique_node_id >= 1 && pCtrlParams->g_unique_node_id <= 8000)
			{
				MutexLockGuard lock(pCtrlParams->g_mapTcpConns_lock);
				std::map<uint64_t, WeakTcpConnectionPtr>::iterator it = pCtrlParams->g_mapTcpConns.find(conn_uuid);
				if (it != pCtrlParams->g_mapTcpConns.end())
				{
					weakconn = it->second;
				}
			}
			else
			{
				if (!conn.empty()) weakconn = boost::any_cast<WeakTcpConnectionPtr>(conn);
			}
			TcpConnectionPtr pConn(weakconn.lock());
			if (pConn && !pConn->getContext().empty())
			{
				ConnNodePtr node(boost::any_cast<ConnNodePtr>(pConn->getContext()));
				if (node)
				{
					m_TcpNetServerImpls[node->GetIndex()]->SetTcpSessionInfo(pConn, sessioninfo);
				}
			}
		}
	}

	// 设置高水位值 (thread safe)
	void SetTcpHighWaterMark(uint64_t conn_uuid, const boost::any& conn, size_t highWaterMark)
	{
		net_ctrl_params_ptr pCtrlParams = m_ptrCtrlParams;
		if (pCtrlParams && !pCtrlParams->g_bExit_tcp) 
		{
			WeakTcpConnectionPtr weakconn;
			if (pCtrlParams->g_unique_node_id >= 1 && pCtrlParams->g_unique_node_id <= 8000)
			{
				MutexLockGuard lock(pCtrlParams->g_mapTcpConns_lock);
				std::map<uint64_t, WeakTcpConnectionPtr>::iterator it = pCtrlParams->g_mapTcpConns.find(conn_uuid);
				if (it != pCtrlParams->g_mapTcpConns.end())
				{
					weakconn = it->second;
				}
			}
			else
			{
				if (!conn.empty()) weakconn = boost::any_cast<WeakTcpConnectionPtr>(conn);
			}
			TcpConnectionPtr pConn(weakconn.lock());
			if (pConn && !pConn->getContext().empty())
			{
				ConnNodePtr node(boost::any_cast<ConnNodePtr>(pConn->getContext()));
				if (node)
				{
					m_TcpNetServerImpls[node->GetIndex()]->SetTcpHighWaterMark(pConn, highWaterMark);
				}
			}
		}
	}
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
// HTTP服务
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// 启动HTTP服务
	void StartHttpServer(void* pInvoker,uint16_t listenport,int threadnum,int idleconntime,int maxconnnum,bool bReusePort,
		pfunc_on_connection pOnConnection, pfunc_on_readmsg_http pOnReadMsg, pfunc_on_sendok pOnSendOk, 
		pfunc_on_senderr pOnSendErr, pfunc_on_highwatermark pOnHighWaterMark, 
		size_t nDefRecvBuf, size_t nMaxRecvBuf, size_t nMaxSendQue)
	{
		LOG_INFO << "HttpServer Start";

		
		threadnum<=0?threadnum=0:1;
		threadnum>128?threadnum=128:1;

		m_ptrCtrlParams->g_nMaxRecvBuf_Http = nMaxRecvBuf;
		m_ptrCtrlParams->g_nMaxConnNum_Http = maxconnnum;

		m_ptrCtrlParams->g_bExit_http  = false;

		m_pHttpLoop = new EventLoop();
		InetAddress listenAddr(listenport, false, false);
		//SO_REUSEPORT
		if (bReusePort)
		{
			m_pHttpEventLoopThreadPool = new EventLoopThreadPool(m_pHttpLoop, "HttpLoop");
			//int nLoopNum = threadnum;
			int nLoopNum = threadnum/2;
			nLoopNum<=0?nLoopNum=1:1;
			nLoopNum>8?nLoopNum=8:1;
			m_pHttpEventLoopThreadPool->setThreadNum(nLoopNum);
			m_pHttpEventLoopThreadPool->start();

			for (int i = 0; i < threadnum; ++i)
			{
				m_HttpNetServerImpls.push_back(new HttpNetServerImpl(pInvoker, static_cast<char>(i & 0xff), m_pHttpEventLoopThreadPool->getNextLoop(), 
					listenAddr, 1, idleconntime, bReusePort, pOnConnection, pOnReadMsg, pOnSendOk, pOnSendErr, pOnHighWaterMark, nDefRecvBuf, nMaxRecvBuf, nMaxSendQue, m_ptrCtrlParams));
			}
		}
		else
		{
			m_HttpNetServerImpls.push_back(new HttpNetServerImpl(pInvoker, 0, m_pHttpLoop, listenAddr, threadnum, idleconntime, bReusePort, 
				pOnConnection, pOnReadMsg, pOnSendOk, pOnSendErr, pOnHighWaterMark, nDefRecvBuf, nMaxRecvBuf, nMaxSendQue, m_ptrCtrlParams));
		}

		//m_pHttpLoop->runEvery(1.0, std::bind(&NetServerCtrlInfo::onIntervalScanf_Http, this));
		m_pHttpLoop->loop();
		delete m_pHttpLoop;
		m_pHttpLoop = NULL;
		LOG_INFO << "HttpServer Quit";
	}

	void StopHttpServerInLoop(const std::shared_ptr<CountDownLatch>& latch)
	{	
		std::vector<HttpNetServerImpl*>::iterator it = m_HttpNetServerImpls.begin();
		for (; it != m_HttpNetServerImpls.end(); ++it)
		{
			(*it)->StopHttpServer();
		}
	
		m_HttpNetServerImpls.clear();
		
		delete m_pHttpEventLoopThreadPool;
		m_pHttpEventLoopThreadPool = NULL;
		
		latch->countDown();
	}
		
	// 关闭HTTP服务
	void StopHttpServer()
	{
		net_ctrl_params_ptr pCtrlParams = m_ptrCtrlParams;
		if (pCtrlParams && !pCtrlParams->g_bExit_http && m_pHttpLoop != NULL)
		{
			std::shared_ptr<CountDownLatch> latch(new CountDownLatch(1));
			pCtrlParams->g_bExit_http = true;
			m_pHttpLoop->runInLoop(std::bind(&NetServerCtrlInfo::StopHttpServerInLoop, this, latch));
			latch->wait();
			m_pHttpLoop->quit();
		}
	}

	// 获取当前HTTP连接总数
	int GetCurrHttpConnNum()
	{
		net_ctrl_params_ptr pCtrlParams = m_ptrCtrlParams;
		if (pCtrlParams) return pCtrlParams->g_nCurrConnNum_Http.get();
		return 0;
	}
	// 获取当前HTTP总请求数
	int64_t GetTotalHttpReqNum()
	{
		net_ctrl_params_ptr pCtrlParams = m_ptrCtrlParams;
		if (pCtrlParams) return pCtrlParams->g_nTotalReqNum_Http.get();
		return 0;
	}
	//获取当前HTTP总的等待发送的数据包数量
	int64_t GetTotalHttpWaitSendNum()
	{
		net_ctrl_params_ptr pCtrlParams = m_ptrCtrlParams;
		if (pCtrlParams) return pCtrlParams->g_nTotalWaitSendNum_http.get();
		return 0;
	}
	// 发送信息
	int SendHttpResponse(uint64_t conn_uuid, const boost::any& conn, const boost::any& params, const HttpResponse& rsp, int timeout)
	{
		int ret = 1;
		net_ctrl_params_ptr pCtrlParams = m_ptrCtrlParams;
		if (pCtrlParams && !pCtrlParams->g_bExit_http)
		{
			WeakTcpConnectionPtr weakconn;
			if (pCtrlParams->g_unique_node_id >= 1 && pCtrlParams->g_unique_node_id <= 8000)
			{
				MutexLockGuard lock(pCtrlParams->g_mapHttpConns_lock);
				std::map<uint64_t, WeakTcpConnectionPtr>::iterator it = pCtrlParams->g_mapHttpConns.find(conn_uuid);
				if (it != pCtrlParams->g_mapHttpConns.end())
				{
					weakconn = it->second;
				}
			}
			else
			{
				if (!conn.empty()) weakconn = boost::any_cast<WeakTcpConnectionPtr>(conn);
			}

			TcpConnectionPtr pConn(weakconn.lock());
			if (pConn && !pConn->getContext().empty())
			{
				ConnNodePtr node(boost::any_cast<ConnNodePtr>(pConn->getContext()));
				if (node)
				{
					ret = m_HttpNetServerImpls[node->GetIndex()]->SendHttpResponse(pConn, params, rsp, timeout);
				}
			}
			// 增加待发出总的计数
			if (0 == ret) pCtrlParams->g_nTotalWaitSendNum_http.increment();
		}
		return ret;
	}
	// 强制关闭HTTP连接
	void ForceCloseHttpConnection(uint64_t conn_uuid, const boost::any& conn)
	{
		net_ctrl_params_ptr pCtrlParams = m_ptrCtrlParams;
		if (pCtrlParams && !pCtrlParams->g_bExit_http)
		{
			WeakTcpConnectionPtr weakconn;
			if (pCtrlParams->g_unique_node_id >= 1 && pCtrlParams->g_unique_node_id <= 8000)
			{
				MutexLockGuard lock(pCtrlParams->g_mapHttpConns_lock);
				std::map<uint64_t, WeakTcpConnectionPtr>::iterator it = pCtrlParams->g_mapHttpConns.find(conn_uuid);
				if (it != pCtrlParams->g_mapHttpConns.end())
				{
					weakconn = it->second;
				}
			}
			else
			{
				if (!conn.empty()) weakconn = boost::any_cast<WeakTcpConnectionPtr>(conn);
			}

			TcpConnectionPtr pConn(weakconn.lock());
			if (pConn && !pConn->getContext().empty())
			{
				ConnNodePtr node(boost::any_cast<ConnNodePtr>(pConn->getContext()));
				if (node)
				{
					m_HttpNetServerImpls[node->GetIndex()]->ForceCloseConnection(pConn);
				}
			}
		}
	}
	// 重置HTTP空连接踢空时间
	void ResetHttpIdleConnTime(int idleconntime)
	{
		net_ctrl_params_ptr pCtrlParams = m_ptrCtrlParams;
		if (pCtrlParams && !pCtrlParams->g_bExit_http)
		{
			std::vector<HttpNetServerImpl*>::iterator itHttp = m_HttpNetServerImpls.begin();
			for (; itHttp != m_HttpNetServerImpls.end(); ++itHttp)
			{
				(*itHttp)->ResetIdleConnTime(idleconntime);
			}
		}
	}

	// 重置HTTP可支持的最大连接数
	void ResetHttpMaxConnNum(int maxconnnum)
	{
		net_ctrl_params_ptr pCtrlParams = m_ptrCtrlParams;
		if (pCtrlParams) pCtrlParams->g_nMaxConnNum_Http = maxconnnum;
	}

	
	// 设置http会话的信息(请传session类的智能指针，并且只允许调用一次&session类中自行控制多线程互斥问题)
	void SetHttpSessionInfo(uint64_t conn_uuid, const boost::any& conn, const boost::any& sessioninfo)
	{
		net_ctrl_params_ptr pCtrlParams = m_ptrCtrlParams;
		if (pCtrlParams && !pCtrlParams->g_bExit_http)
		{
			WeakTcpConnectionPtr weakconn;
			if (pCtrlParams->g_unique_node_id >= 1 && pCtrlParams->g_unique_node_id <= 8000)
			{
				MutexLockGuard lock(pCtrlParams->g_mapHttpConns_lock);
				std::map<uint64_t, WeakTcpConnectionPtr>::iterator it = pCtrlParams->g_mapHttpConns.find(conn_uuid);
				if (it != pCtrlParams->g_mapHttpConns.end())
				{
					weakconn = it->second;
				}
			}
			else
			{
				if (!conn.empty()) weakconn = boost::any_cast<WeakTcpConnectionPtr>(conn);
			}

			TcpConnectionPtr pConn(weakconn.lock());
			if (pConn && !pConn->getContext().empty())
			{
				ConnNodePtr node(boost::any_cast<ConnNodePtr>(pConn->getContext()));
				if (node)
				{
					m_HttpNetServerImpls[node->GetIndex()]->SetHttpSessionInfo(pConn, sessioninfo);
				}
			}
		}
	}

	// 设置高水位值 (thread safe)
	void SetHttpHighWaterMark(uint64_t conn_uuid, const boost::any& conn, size_t highWaterMark)
	{
		net_ctrl_params_ptr pCtrlParams = m_ptrCtrlParams;
		if (pCtrlParams && !pCtrlParams->g_bExit_http)
		{
			WeakTcpConnectionPtr weakconn;
			if (pCtrlParams->g_unique_node_id >= 1 && pCtrlParams->g_unique_node_id <= 8000)
			{
				MutexLockGuard lock(pCtrlParams->g_mapHttpConns_lock);
				std::map<uint64_t, WeakTcpConnectionPtr>::iterator it = pCtrlParams->g_mapHttpConns.find(conn_uuid);
				if (it != pCtrlParams->g_mapHttpConns.end())
				{
					weakconn = it->second;
				}
			}
			else
			{
				if (!conn.empty()) weakconn = boost::any_cast<WeakTcpConnectionPtr>(conn);
			}

			TcpConnectionPtr pConn(weakconn.lock());
			if (pConn && !pConn->getContext().empty())
			{
				ConnNodePtr node(boost::any_cast<ConnNodePtr>(pConn->getContext()));
				if (node)
				{
					m_HttpNetServerImpls[node->GetIndex()]->SetHttpHighWaterMark(pConn, highWaterMark);
				}
			}
		}
	}
private:
	// 主线程定时扫描
	void onIntervalScanf_Tcp()
	{
		net_ctrl_params_ptr pCtrlParams = m_ptrCtrlParams;
		if (pCtrlParams)
		{
			int64_t nReqTmp_Tcp = pCtrlParams->g_nTotalReqNum_Tcp.get();
			int64_t nLastReqTmp_Tcp = pCtrlParams->g_nLastTotalReqNum_Tcp.get();

			printf("TcpServer::CurrConn:%d,TotalReq:%ld,Spd:%ld\n", pCtrlParams->g_nCurrConnNum_Tcp.get(), nReqTmp_Tcp, nReqTmp_Tcp - nLastReqTmp_Tcp);

			pCtrlParams->g_nLastTotalReqNum_Tcp.getAndSet(nReqTmp_Tcp);
		}
	}
	// 主线程定时扫描
	void onIntervalScanf_Http()
	{
		net_ctrl_params_ptr pCtrlParams = m_ptrCtrlParams;
		if (pCtrlParams)
		{
			int64_t nReqTmp_Http = pCtrlParams->g_nTotalReqNum_Http.get();
			int64_t nLastReqTmp_Http = pCtrlParams->g_nLastTotalReqNum_Http.get();

			printf("HttpServer::CurrConn:%d,TotalReq:%ld,Spd:%ld\n", pCtrlParams->g_nCurrConnNum_Http.get(), nReqTmp_Http, nReqTmp_Http - nLastReqTmp_Http);

			pCtrlParams->g_nLastTotalReqNum_Http.getAndSet(nReqTmp_Http);
		}
	}
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
public:
	// ------TCP 服务端的loop
	EventLoop* m_pTcpLoop;
	EventLoopThreadPool* m_pTcpEventLoopThreadPool;
	std::vector<TcpNetServerImpl*> m_TcpNetServerImpls;

	// ------HTTP 服务端的loop
	EventLoop* m_pHttpLoop;
	EventLoopThreadPool* m_pHttpEventLoopThreadPool;
	std::vector<HttpNetServerImpl*> m_HttpNetServerImpls;

	uint16_t m_unique_node_id;

	net_ctrl_params_ptr m_ptrCtrlParams;
};

//----------------------------------------------------------------------------------------------------------------

NetServer::NetServer(uint16_t unique_node_id):
m_NetServerCtrl(new NetServerCtrlInfo())
{
	assert(unique_node_id>=1 && unique_node_id<=8000);

	net_ctrl_params_ptr  ptrParams = m_NetServerCtrl->m_ptrCtrlParams;

	ptrParams->g_unique_node_id = unique_node_id;

	//暂时取消
	mkdir("./netlogs", 0755);
	if (!g_logFile) g_logFile.reset(new mwnet_mt::LogFile("./netlogs/", "netserverlog", 1024*1024*100, true, 0, 1));
  	mwnet_mt::Logger::setOutput(outputFunc);
  	mwnet_mt::Logger::setFlush(flushFunc);

	//默认启用日志
	ptrParams->g_bEnableRecvLog_http = true;
	ptrParams->g_bEnableSendLog_http = true;
	ptrParams->g_bEnableRecvLog_tcp = true;
	ptrParams->g_bEnableSendLog_tcp = true;
}

//设置nodeid(1~8000间取值)
void NetServer::SetUniqueNodeId(uint16_t unique_node_id)
{
	assert(unique_node_id>=1 && unique_node_id<=8000);

	if (m_NetServerCtrl)
	{
		net_ctrl_params_ptr pCtrlParams = m_NetServerCtrl->m_ptrCtrlParams;
		if (pCtrlParams) pCtrlParams->g_unique_node_id = unique_node_id;
	}
}

NetServer::NetServer():
m_NetServerCtrl(new NetServerCtrlInfo())
{
	net_ctrl_params_ptr  ptrParams = m_NetServerCtrl->m_ptrCtrlParams;
	ptrParams->g_unique_node_id = 0;

	//暂时取消
	mkdir("./netlogs", 0755);
	if (!g_logFile) g_logFile.reset(new mwnet_mt::LogFile("./netlogs/", "netserverlog", 1024*1024*100, true, 0, 1));
  	mwnet_mt::Logger::setOutput(outputFunc);
  	mwnet_mt::Logger::setFlush(flushFunc);

	//默认启用日志
	ptrParams->g_bEnableRecvLog_http = true;
	ptrParams->g_bEnableSendLog_http = true;
	ptrParams->g_bEnableRecvLog_tcp = true;
	ptrParams->g_bEnableSendLog_tcp = true;
}

NetServer::~NetServer()
{
	StopTcpServer();
	StopHttpServer();
}

// 开启或关闭接收/发送日志
void NetServer::EnableHttpServerRecvLog(bool bEnable)
{
	if (m_NetServerCtrl)
	{
		net_ctrl_params_ptr pCtrlParams = m_NetServerCtrl->m_ptrCtrlParams;
		if (pCtrlParams) pCtrlParams->g_bEnableRecvLog_http = bEnable;
	}
}

void NetServer::EnableHttpServerSendLog(bool bEnable)
{
	if (m_NetServerCtrl)
	{
		net_ctrl_params_ptr pCtrlParams = m_NetServerCtrl->m_ptrCtrlParams;
		if (pCtrlParams) pCtrlParams->g_bEnableSendLog_http = bEnable;
	}
}

void NetServer::EnableTcpServerRecvLog(bool bEnable)
{
	if (m_NetServerCtrl)
	{
		net_ctrl_params_ptr pCtrlParams = m_NetServerCtrl->m_ptrCtrlParams;
		if (pCtrlParams) pCtrlParams->g_bEnableRecvLog_tcp = bEnable;
	}
}

void NetServer::EnableTcpServerSendLog(bool bEnable)
{
	if (m_NetServerCtrl)
	{
		net_ctrl_params_ptr pCtrlParams = m_NetServerCtrl->m_ptrCtrlParams;
		if (pCtrlParams) pCtrlParams->g_bEnableSendLog_tcp = bEnable;
	}
}

// ------------------------------------------------TcpServer Begin-----------------------------------------------------
// 启动TCP服务
void NetServer::StartTcpServer(void* pInvoker,uint16_t listenport,int threadnum,int idleconntime,int maxconnnum,bool bReusePort,
	pfunc_on_connection pOnConnection, pfunc_on_readmsg_tcp pOnReadMsg, pfunc_on_sendok pOnSendOk, 
	pfunc_on_senderr pOnSendErr, pfunc_on_highwatermark pOnHighWaterMark,
	size_t nDefRecvBuf, size_t nMaxRecvBuf, size_t nMaxSendQue)
{
	m_NetServerCtrl->StartTcpServer(pInvoker,listenport,threadnum,idleconntime,maxconnnum,bReusePort,pOnConnection,
		pOnReadMsg,pOnSendOk, pOnSendErr,pOnHighWaterMark,nDefRecvBuf,nMaxRecvBuf,nMaxSendQue);
}
// 关闭TCP服务
void NetServer::StopTcpServer()
{
	if (m_NetServerCtrl) m_NetServerCtrl->StopTcpServer();
}
// 获取当前TCP连接总数
int NetServer::GetCurrTcpConnNum()
{
	if (m_NetServerCtrl) return m_NetServerCtrl->GetCurrTcpConnNum();
	return 0;
}
// 获取当前TCP总请求数
int64_t NetServer::GetTotalTcpReqNum()
{
	if (m_NetServerCtrl) return m_NetServerCtrl->GetTotalTcpReqNum();
	return 0;
}
//获取当前TCP总的等待发送的数据包数量
int64_t NetServer::GetTotalTcpWaitSendNum()
{
	if (m_NetServerCtrl) return m_NetServerCtrl->GetTotalTcpWaitSendNum();
	return 0;
}

// 发送信息
int NetServer::SendMsgWithTcpServer(uint64_t conn_uuid, const boost::any& conn, const boost::any& params, const char* szMsg, size_t nMsgLen, int timeout, bool bKeepAlive)
{
	if (m_NetServerCtrl) return m_NetServerCtrl->SendMsgWithTcpServer(conn_uuid, conn, params, szMsg, nMsgLen, timeout, bKeepAlive);
	return 1;
}
//强制关闭TCP连接
void NetServer::ForceCloseTcpConnection(uint64_t conn_uuid, const boost::any& conn)
{
	if (m_NetServerCtrl) m_NetServerCtrl->ForceCloseTcpConnection(conn_uuid, conn);
}
// 重置TCP空连接踢空时间
void NetServer::ResetTcpIdleConnTime(int idleconntime)
{
	if (m_NetServerCtrl) m_NetServerCtrl->ResetTcpIdleConnTime(idleconntime);
}

// 重置TCP可支持的最大连接数
void NetServer::ResetTcpMaxConnNum(int maxconnnum)
{
	if (m_NetServerCtrl) m_NetServerCtrl->ResetTcpMaxConnNum(maxconnnum);
}

//设置tcp会话的信息(请传session类的智能指针，并且只允许调用一次&session类中自行控制多线程互斥问题)
void NetServer::SetTcpSessionInfo(uint64_t conn_uuid, const boost::any& conn, const boost::any& sessioninfo)
{
	if (m_NetServerCtrl) m_NetServerCtrl->SetTcpSessionInfo(conn_uuid, conn, sessioninfo);
}

//设置高水位值 (thread safe)
void NetServer::SetTcpHighWaterMark(uint64_t conn_uuid, const boost::any& conn, size_t highWaterMark)
{
	if (m_NetServerCtrl) m_NetServerCtrl->SetTcpHighWaterMark(conn_uuid, conn, highWaterMark);
}

//-------------------------------------------TcpServer End----------------------------------------------------


//-------------------------------------------HttpServer Begin-------------------------------------------------
// 启动HTTP服务
void NetServer::StartHttpServer(void* pInvoker,uint16_t listenport,int threadnum,int idleconntime,int maxconnnum,bool bReusePort,
	pfunc_on_connection pOnConnection, pfunc_on_readmsg_http pOnReadMsg, pfunc_on_sendok pOnSendOk, 
	pfunc_on_senderr pOnSendErr,pfunc_on_highwatermark pOnHighWaterMark, 
	size_t nDefRecvBuf, size_t nMaxRecvBuf, size_t nMaxSendQue)
{
	if (m_NetServerCtrl) 
	m_NetServerCtrl->StartHttpServer(pInvoker,listenport,threadnum,idleconntime,maxconnnum,bReusePort,pOnConnection,
		pOnReadMsg,pOnSendOk,pOnSendErr,pOnHighWaterMark,nDefRecvBuf,nMaxRecvBuf,nMaxSendQue);
}
// 关闭HTTP服务
void NetServer::StopHttpServer()
{
	if (m_NetServerCtrl) m_NetServerCtrl->StopHttpServer();
}
// 获取当前HTTP连接总数
int NetServer::GetCurrHttpConnNum()
{
	if (m_NetServerCtrl) return m_NetServerCtrl->GetCurrHttpConnNum();
	return 0;
}
// 获取当前HTTP总请求数
int64_t NetServer::GetTotalHttpReqNum()
{
	if (m_NetServerCtrl) return m_NetServerCtrl->GetTotalHttpReqNum();
	return 0;
}
//获取当前HTTP总的等待发送的数据包数量
int64_t NetServer::GetTotalHttpWaitSendNum()
{
	if (m_NetServerCtrl) return m_NetServerCtrl->GetTotalHttpWaitSendNum();
	return 0;
}

// 发送HTTP回应信息
int NetServer::SendHttpResponse(uint64_t conn_uuid, const boost::any& conn, const boost::any& params, const HttpResponse& rsp, int timeout)
{
	if (m_NetServerCtrl) return m_NetServerCtrl->SendHttpResponse(conn_uuid, conn, params, rsp, timeout);
	return 1;
}
//强制关闭HTTP连接
void NetServer::ForceCloseHttpConnection(uint64_t conn_uuid, const boost::any& conn)
{
	if (m_NetServerCtrl) m_NetServerCtrl->ForceCloseHttpConnection(conn_uuid, conn);
}

// 重置HTTP空连接踢空时间
void NetServer::ResetHttpIdleConnTime(int idleconntime)
{
	if (m_NetServerCtrl) m_NetServerCtrl->ResetHttpIdleConnTime(idleconntime);
}

// 重置HTTP可支持的最大连接数
void NetServer::ResetHttpMaxConnNum(int maxconnnum)
{
	if (m_NetServerCtrl) m_NetServerCtrl->ResetHttpMaxConnNum(maxconnnum);
}

//设置http会话的信息(请传session类的智能指针，并且只允许调用一次&session类中自行控制多线程互斥问题)
void NetServer::SetHttpSessionInfo(uint64_t conn_uuid, const boost::any& conn, const boost::any& sessioninfo)
{
	if (m_NetServerCtrl) m_NetServerCtrl->SetHttpSessionInfo(conn_uuid, conn, sessioninfo);
}

//设置高水位值 (thread safe)
void NetServer::SetHttpHighWaterMark(uint64_t conn_uuid, const boost::any& conn, size_t highWaterMark)
{
	if (m_NetServerCtrl) m_NetServerCtrl->SetHttpHighWaterMark(conn_uuid, conn, highWaterMark);
}

//-------------------------------------------HttpServer End----------------------------------------------------

}
}
