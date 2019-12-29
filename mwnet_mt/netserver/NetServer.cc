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

#include <mwnet_mt/util/MWStringUtil.h>

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
using namespace MWSTRINGUTIL;

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

// ʮ�������ַ�ת��������
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
	char m_index;							// server������
	time_t m_tLastUpd;						// ���һ�λʱ��
	WeakConnectionList::iterator m_nodepos;	// �����е�λ��
};
typedef std::shared_ptr<ConnNode> ConnNodePtr;
/*
enum MODULE_LOGS
{
	MODULE_LOGS_DEFAULT,	//Ĭ��ģ��,����ģ��
	MODULE_LOG_NUMS,		//��־����
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
	// ȫ�ֱ���
	int g_nMaxConnNum_Tcp;				// ֧�ֵ����������
	AtomicInt32 g_nCurrConnNum_Tcp; 	// ��ǰ������
	AtomicInt64 g_nTotalReqNum_Tcp; 	// �ܵ�������
	AtomicInt64 g_nLastTotalReqNum_Tcp; // �ϴε���������
	MutexLock g_mapTcpConns_lock;
	std::map<uint64_t, WeakTcpConnectionPtr> g_mapTcpConns;

	int g_nMaxConnNum_Http; 				// ֧�ֵ����������
	AtomicInt32 g_nCurrConnNum_Http;		// ��ǰ������
	AtomicInt64 g_nTotalReqNum_Http;		// �ܵ�������
	AtomicInt64 g_nLastTotalReqNum_Http;	// �ϴε���������
	uint16_t g_unique_node_id;

	AtomicInt64 g_nSequnceId;
	MutexLock g_mapHttpConns_lock;
	std::map<uint64_t, WeakTcpConnectionPtr> g_mapHttpConns;

	bool g_bEnableRecvLog_http;					//������־����_http
	bool g_bEnableSendLog_http;					//������־����_http
	bool g_bEnableRecvLog_tcp;					//������־����_tcp
	bool g_bEnableSendLog_tcp;					//������־����_tcp

	size_t g_nMaxRecvBuf_Tcp;					//tcp�����ջ���
	size_t g_nMaxRecvBuf_Http;					//http�����ջ���

	AtomicInt64 g_nTotalWaitSendNum_http;		//http�ȴ����ͳ��İ�������
	AtomicInt64 g_nTotalWaitSendNum_tcp;		//tcp�ȴ����ͳ��İ�������

	bool g_bExit_http;							//http�����˳���־
	bool g_bExit_tcp;							//http�����˳���־
}net_ctrl_params;

typedef std::shared_ptr<net_ctrl_params> net_ctrl_params_ptr;

//������ڲ�͸������,���ڷ���ʱ�����ϲ㴫����any����,��Ϊÿһ�η��������һ��Ωһ������ID,��onwriteok/error�ص�ʱ����׷�ٸ�����
struct tInnerParams
{
	uint64_t request_id;
	boost::any params;
	tInnerParams(uint64_t x, boost::any y)
	{
		request_id = x; params = y;
	}
};

//-----------------------------------------------------------

//��־
//..........................................................
std::unique_ptr<mwnet_mt::LogFile> g_logFile;

void outputFunc(const char* msg, size_t len)
{
  g_logFile->append(msg, len);
}

void flushFunc()
{
  g_logFile->flush();
}
//........................................................
//��λԤ��,��Զ����,�������֧��8000�����,ÿ�����1���������16777215�����ظ���ID,����֤1���ڲ��ظ�.
uint64_t MakeSnowId(uint16_t unique_node_id, AtomicInt64& nSequnceId)
{
	unsigned int sign = 0;
	unsigned int nYear = 0;
	unsigned int nMonth = 0;
	unsigned int nDay = 0;
	unsigned int nHour = 0;
	unsigned int nMin = 0;
	unsigned int nSec = 0;
	unsigned int nNodeid = unique_node_id;
	unsigned int nNo = static_cast<unsigned int>(nSequnceId.getAndAdd(1) % (0xFFFFFF));

  	struct timeval tv;
	struct tm	   tm_time;
	gettimeofday(&tv, NULL);
	localtime_r(&tv.tv_sec, &tm_time);

	nYear = tm_time.tm_year+1900;
	nYear = nYear%100;
	nMonth = tm_time.tm_mon+1;
	nDay = tm_time.tm_mday;
	nHour = tm_time.tm_hour;
	nMin = tm_time.tm_min;
	nSec = tm_time.tm_sec;

	int64_t j = 0;
	j |= static_cast<int64_t>(sign) << 63;   //����λ 0
	j |= static_cast<int64_t>(nMonth & 0x0f) << 59;//month 1~12
	j |= static_cast<int64_t>(nDay & 0x1f) << 54;//day 1~31
	j |= static_cast<int64_t>(nHour & 0x1f) << 49;//hour 0~24
	j |= static_cast<int64_t>(nMin & 0x3f) << 43;//min 0~59
	j |= static_cast<int64_t>(nSec & 0x3f) << 37;//second 0~59
	j |= static_cast<int64_t>(nNodeid & 0x01fff) << 24;//nodeid 1~8000
	j |= static_cast<int64_t>(nNo & 0xFFFFFF);	//seqid,0~0xFFFFFF 0~16777215

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
		// �������¼��ص�(connect,disconnect)
		m_server.setConnectionCallback(std::bind(&TcpNetServerImpl::onConnection, this, _1));
		// ���յ������ݻص�
		m_server.setMessageCallback(std::bind(&TcpNetServerImpl::onMessage, this, _1, _2, _3, std::placeholders::_4));
		// ��д������ɻص�
		m_server.setWriteCompleteCallback(std::bind(&TcpNetServerImpl::onWriteOk, this, _1, _2));
		// ��д�����쳣��ʧ�ܻص�
		m_server.setWriteErrCallback(std::bind(&TcpNetServerImpl::onWriteErr, this, _1, _2, _3));
		// ���߳������ɹ��ص�
		m_server.setThreadInitCallback(std::bind(&TcpNetServerImpl::onThreadInit, this, _1));
		// ���ù����߳���
		m_server.setThreadNum(threadnum);
		// ����server
		m_loop->runInLoop(std::bind(&TcpNetServerImpl::start, this));
	}

	// ���ÿ������߿�ʱ��
	void ResetIdleConnTime(int idleconntime)
	{
		m_nIdleConnTime = idleconntime;
	}

	// ������Ϣ
	int OnSendMsg(const TcpConnectionPtr& conn, const boost::any& params, const char* szMsg, size_t nMsgLen, int timeout, bool bKeepAlive)
	{
		uint64_t request_id = MakeSnowId(m_ptrCtrlParams->g_unique_node_id, m_ptrCtrlParams->g_nSequnceId);
		tInnerParams inner_params(request_id, params);
		boost::any inner_any(inner_params);

		if (m_ptrCtrlParams->g_bEnableRecvLog_tcp)
		{
			LOG_INFO << "[TCP][SENDING]"
				<< "[" << request_id << "]"
				<< "[" << conn->getConnuuid() << "]"
				<< "[" << conn->peerAddress().toIpPort() << "]"
				<< ":" << StringUtil::BytesToHexString(szMsg, nMsgLen);
		}

		if (!bKeepAlive) conn->setNeedCloseFlag();
		int nSendRet = conn->send(szMsg, static_cast<int>(nMsgLen), inner_any, timeout);

		if (m_ptrCtrlParams->g_bEnableRecvLog_tcp && 0 != nSendRet)
		{
			LOG_WARN << "[TCP][SENDERR]"
				<< "[" << request_id << "]"
				<< "[" << conn->getConnuuid() << "]"
				<< "[" << conn->peerAddress().toIpPort() << "]"
				<< ":" << "SEND ERROR,ERCODE:" << nSendRet;
		}

		return nSendRet;
	}

	// ǿ�ƹر�����
	void ForceCloseConnection(const TcpConnectionPtr& conn)
	{
		conn->shutdown();
		//conn->forceCloseWithDelay(1.0);  // > round trip of the whole Internet.
		conn->forceClose();
	}
	//��ͣ��ǰTCP���ӵ����ݽ���
	void SuspendConnRecv(const TcpConnectionPtr& conn, int delay)
	{
		conn->suspendRecv(delay);
	}
	//�ָ���ǰTCP���ӵ����ݽ���
	void ResumeConnRecv(const TcpConnectionPtr& conn)
	{
		conn->resumeRecv();
	}
	// ����tcp�Ự����Ϣ(�봫session�������ָ�룬����ֻ�������һ��&session�������п��ƶ��̻߳�������)
	void SetTcpSessionInfo(const TcpConnectionPtr& conn, const boost::any& sessioninfo)
	{
		conn->setSessionData(sessioninfo);
	}

	// ���ø�ˮλֵ
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
			uint64_t conn_uuid = MakeSnowId(m_ptrCtrlParams->g_unique_node_id, m_ptrCtrlParams->g_nSequnceId);
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
						//��Ϊ��Чpos
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

			// �������������
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

	void onMessage(const TcpConnectionPtr& conn, Buffer* buf, size_t len, Timestamp time)
	{
		if (!conn) return;

		// write logs
		if (m_ptrCtrlParams->g_bEnableRecvLog_tcp)
		{
			uint64_t request_id = MakeSnowId(m_ptrCtrlParams->g_unique_node_id, m_ptrCtrlParams->g_nSequnceId);
			const char* pRecv = buf->reverse_peek(len);
			LOG_INFO << "[TCP][RECVOK ]"
				<< "[" << request_id << "]"
				<< "[" << conn->getConnuuid() << "]"
				<< "[" << conn->peerAddress().toIpPort() << "]"
				<< ":" << (pRecv ? StringUtil::BytesToHexString(pRecv, len) : "RECV NULL");
		}

		m_ptrCtrlParams->g_nTotalReqNum_Tcp.increment();

		UpdateConnList(conn, false);

		if (m_pfunc_on_readmsg_tcp)
		{
			int nReadedLen = 0;
			StringPiece RecvMsg = buf->toStringPiece();

			WeakTcpConnectionPtr weakconn(conn);
			int nRet = m_pfunc_on_readmsg_tcp(m_pInvoker, conn->getConnuuid(), weakconn, conn->getSessionData(), RecvMsg.data(), RecvMsg.size(), nReadedLen);
			// �պö���������
			if (nReadedLen == RecvMsg.size())
			{
				buf->retrieveAll();
			}
			// ճ��/�а�
			else
			{
				// ��buf�е�ָ��������ǰ�Ѵ����ĵط�
				buf->retrieve(nReadedLen);

				// ��û�г���������BUF,�����κδ���...�Ƚ��������ٴ���,������,��ֱ��close
				if (buf->readableBytes() >= m_ptrCtrlParams->g_nMaxRecvBuf_Tcp)
				{
					nRet = -1;
					LOG_WARN << "[TCP][RECVERR]"
						<< "[" << conn->getConnuuid() << "]"
						<< ":illegal data size " << buf->readableBytes() 
						<< ",larger than " << m_ptrCtrlParams->g_nMaxRecvBuf_Tcp << ",maybe too long\n";
				}
			}
			// �жϷ���ֵ��������ͬ�Ĵ���
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

		tInnerParams inner_params(boost::any_cast<tInnerParams>(params));

		if (m_ptrCtrlParams->g_bEnableRecvLog_tcp)
		{
			LOG_INFO << "[TCP][SENDOK ]"
				<< "[" << inner_params.request_id << "]"
				<< "[" << conn->getConnuuid() << "]"
				<< "[" << conn->peerAddress().toIpPort() << "]";
		}

		// �����ܴ���������
		m_ptrCtrlParams->g_nTotalWaitSendNum_tcp.decrement();

		int nRet = 0;
		if (m_pfunc_on_sendok)
		{
			WeakTcpConnectionPtr weakconn(conn);
			nRet = m_pfunc_on_sendok(m_pInvoker, conn->getConnuuid(), weakconn, conn->getSessionData(), inner_params.params);
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

		tInnerParams inner_params(boost::any_cast<tInnerParams>(params));
		
		if (m_ptrCtrlParams->g_bEnableRecvLog_tcp)
		{
			LOG_ERROR << "[TCP][SENDERR]"
				<< "[" << inner_params.request_id << "]"
				<< "[" << conn->getConnuuid() << "]"
				<< "[" << conn->peerAddress().toIpPort() << "]"
				<< ":" << "SEND ERROR,ERCODE:" << errCode;
		}

		// �����ܴ���������
		m_ptrCtrlParams->g_nTotalWaitSendNum_tcp.decrement();

		int nRet = 0;
		if (m_pfunc_on_senderr)
		{
			WeakTcpConnectionPtr weakconn(conn);
			nRet = m_pfunc_on_senderr(m_pInvoker, conn->getConnuuid(), weakconn, conn->getSessionData(), inner_params.params, errCode);
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

	// ������ʱ��
	void startTimer()
	{
		EventLoop* loop = m_server.threadPool()->getNextLoop();
		loop->runInLoop(std::bind(&TcpNetServerImpl::InitConnList, this));
		loop->runEvery(1.0, std::bind(&TcpNetServerImpl::onCheckConnTimeOut, this));
	}

	// ��ʼ�����Ӷ���
	void InitConnList()
	{
		ThreadLocalSingleton<WeakConnectionList>::instance();
	}

	// ��ʱ��,��ʱ��ⳬʱ
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
					// ��ʱ
					if (nTmDiff > m_nIdleConnTime)
					{
						if (conn->connected())
						{
							LOG_INFO<<"IP:["<<conn->peerAddress().toIpPort()<<"] CONNUUID:["<<conn->getConnuuid()<<"] "<<nTmDiff<<" s NO ACTIVE,KICK IT OUT.\n";
							// �߳�
							conn->shutdown();
							//conn->forceCloseWithDelay(1.0);  // > round trip of the whole Internet.
							conn->forceClose();
						}
					}
					// ʱ��������
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
				// ������ʧЧ
				connlist.erase(it++);
			}
		}
	}

private:
	void* m_pInvoker;
	char m_index;
	EventLoop* m_loop;
	TcpServer m_server;
	int m_nIdleConnTime;			// �����ӳ�ʱ�߳�ʱ��
	size_t m_nHighWaterMark;	    // ��ˮλֵ
	
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
// �Ƿ���Ҫ��������,��connection��keep-alive����close
// ����ֵ:true:keep-alive false:close
bool HttpRequest::IsKeepAlive() const
{
	HttpParser* psr = reinterpret_cast<HttpParser* >(p);
	if (psr)
	{
		return psr->should_keep_alive();
	}
	return false;
}

//��������http����ı��ĳ���(����ͷ+������)
size_t	HttpRequest::GetHttpRequestTotalLen() const
{
	HttpParser* psr = reinterpret_cast<HttpParser* >(p);
	if (psr)
	{
		return psr->get_http_total_req_len();
	}
	return 0;

}

// ��ȡhttp body������
// ����ֵ:body�����ݺͳ���
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

// ��ȡ����url
// ����ֵ:url�������Լ�����
// ˵��:��:post����,POST /sms/std/single_send HTTP/1.1 ,/sms/std/single_send����URL
//	   ��:get����,
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
// ��ȡ����ͻ��˵�IP
// ����ֵ:����ͻ��˵�IP
std::string HttpRequest::GetClientRequestIp() const
{
	return m_strReqIp;
}

// ��ȡ����ͻ��˵Ķ˿�
// ����ֵ:����ͻ��˵Ķ˿�
uint16_t HttpRequest::GetClientRequestPort() const
{
	return m_uReqPort;
}

// ��ȡUser-Agent
// ����ֵ:User-Agent�������Լ�����
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

// ��ȡcontent-type
// ����ֵ:CONTENT_TYPE_XML ���� xml,CONTENT_TYPE_JSON ���� json,CONTENT_TYPE_URLENCODE ���� urlencode,CONTENT_TYPE_UNKNOWN ���� δ֪����
// ˵��:��ֻ�ṩ������
int HttpRequest::GetContentType() const
{
	HttpParser* psr = reinterpret_cast<HttpParser* >(p);
	if (psr)
	{
		return psr->get_http_content_type();
	}
	return CONTENT_TYPE_UNKNOWN;
}

// ��ȡhost
// ����ֵ:host�������Լ�����
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

// ��ȡx-forwarded-for
// ����ֵ:X-Forwarded-For�������Լ�����
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

// ��ȡhttp���������
// ����ֵ:HTTP_REQUEST_POST ���� post����; HTTP_REQUEST_GET ���� get���� ; HTTP_REQUEST_UNKNOWN ���� δ֪(������ͷ�д���)
int HttpRequest::GetRequestType() const
{
	HttpParser* psr = reinterpret_cast<HttpParser* >(p);
	if (psr)
	{
		return psr->get_http_request_type();
	}
	return HTTP_REQUEST_UNKNOWN;
}

// ��ȡSOAPAction(������soapʱ������)
// ����ֵ:SOAPAction�����Լ�����
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

//��ȡWap Via
//����ֵ:via�����Լ�����
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

//��ȡx-wap-profile
//����ֵ:x-wap-profile�����Լ�����
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

//��ȡx-browser-type
//����ֵ:x-browser-type�����Լ�����
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

//��ȡx-up-bear-type
//����ֵ:x-up-bear-type�����Լ�����
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

//��ȡhttpͷ����һ�����Ӧ��ֵ(�����ִ�Сд)
//����ֵ:���Ӧ��ֵ
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

//��ȡ����http���ĵ�����(ͷ+��)
//����ֵ:http���ĵ�ָ��,�Լ�����
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

// ..........................�����ṩһ�����urlencode�ĺ���......................
// ����key,value��ʽ��body,��:cmd=aa&seqid=1&msg=11
// ע��:����GetContentType����CONTENT_TYPE_URLENCODE�ſ���ʹ�øú���ȥ����
bool HttpRequest::ParseUrlencodedBody(const char* pbody/*http body ��:cmd=aa&seqid=1&msg=11*/, size_t len/*http body�ĳ���*/) const
{
	HttpParser* psr = reinterpret_cast<HttpParser* >(p);
	if (psr)
	{
		return psr->parse_http_body(pbody, len);
	}
	return false;
}

// ͨ��key��ȡvalue��ָ��ͳ���,�������忽��.������false����key������
// ע��:�����ȵ���ParseUrlencodedBody��ſ���ʹ�øú���
bool HttpRequest::GetUrlencodedBodyValue(const char* key/*key,��:cmd=aa,cmd����key,aa����value*/, std::string& value) const
{
	HttpParser* psr = reinterpret_cast<HttpParser* >(p);
	if (psr)
	{
		return psr->get_value(key, value);
	}
	return false;
}
//................................................................................
//����һЩ���Ʊ�־
void HttpRequest::ResetSomeCtrlFlag()
{
	m_bHasRsp100Continue = false;
	m_bIncomplete = false;
	m_strRequest = "";
}

//���Ѿ��ظ���100-continue��־
void HttpRequest::SetHasRsp100Continue()
{
	m_bHasRsp100Continue = true;
}

//�Ƿ���Ҫ�ظ�100continue;
bool HttpRequest::HasRsp100Continue()
{
	return m_bHasRsp100Continue;
}

//�����ѽ��յ�������
void HttpRequest::SaveIncompleteData(const char* pData, size_t nLen)
{
	m_strRequest.append(pData, nLen);
}

//���ý�����
void HttpRequest::ResetParser()
{
	HttpParser* psr = reinterpret_cast<HttpParser* >(p);
	if (psr)
	{
		psr->reset_parser();
	}
}

//����ͷ���Ƿ���100-continue;
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

// ����http����
// ����ֵ 0:�ɹ� ��ʱҪ��over�Ƿ�Ϊtrue,��Ϊtrue����������,��Ϊfalse�����δ��ȫ
// ����ֵ 1:�������ʧ�� ��ر�����
// ����ֵ 2:���������,��ر�����
int	HttpRequest::ParseHttpRequest(const char* data, size_t len, size_t maxlen, bool& over)
{
	int nRet = 1;

	HttpParser* parser = reinterpret_cast<HttpParser* >(p);
	if (parser)
	{
		//���ƴ���ĳ���
		if (len <= 0 || len > maxlen) 
		{
			return nRet = 1;
		}

		//������ݸ�����ͷ����"\r","\n"ֱ�Ӿܾ�,����ǲа�,���������
		if ((data[0] == '\r' || data[0] == '\n') && !m_bIncomplete)
		{
			return nRet = 1;
		}
		
		// �����δ�������İ�,�ȸ����ϵ��ϴα���Ĳ���,Ȼ����ʹ�ô洢�Ĳ��ֽ��н���
		if (m_bIncomplete) SaveIncompleteData(data, len);
		nRet = parser->execute_http_parse(m_bIncomplete?m_strRequest.c_str():data, m_bIncomplete?m_strRequest.size():len, over);
		// ��û���꣬�����գ����ûص�, �����뱣������, �����ñ�־�ð���ʹ���ϴα������������ݽ����´ν���
		if (!m_bIncomplete && 0 == nRet && !over)
		{
			SaveIncompleteData(data, len);
			m_bIncomplete = true;
		}

		// ��������maxlen��������Ȼ����һ��������,��ʱ�ر�����
		if (!over && m_strRequest.size() >= maxlen)
		{
			nRet = 2;
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

// ����״̬�룬200��400��404 etc.
// ע��:�ú����ڲ���У�飬ֻ�������һ��
void HttpResponse::SetStatusCode(int nStatusCode)
{
	if (!m_bHaveSetStatusCode)
	{
		m_nStatusCode = nStatusCode;
		if (200 != m_nStatusCode && 202 != m_nStatusCode && 100 != m_nStatusCode)
		{
			m_bKeepAlive = false;// �ǳɹ�������ʱ��ǿ�ƽ�������Ϊ��������
			FormatHttpResponse(g_statuscoderesson.GetStatusCodeReason(m_nStatusCode));
		}
	}
	m_bHaveSetStatusCode = true;
}

// ����conetnettype���磺text/json��application/x-www-form-urlencoded��text/xml
// ע��:�ú����ڲ���У�飬ֻ�������һ��
void HttpResponse::SetContentType(const std::string& strContentType)
{
	if (!m_bHaveSetContentType)
	{
		m_strContentType = strContentType;
	}
	m_bHaveSetContentType = true;
}


//����Location���磺��״̬��Ϊ301��302����Э����תʱ����Ҫ���øò���ָ����ת��ַ
//ע��:�ú����ڲ���У�飬ֻ�������һ��
void HttpResponse::SetLocation(const std::string& strLocation)
{
	if (!m_bHaveSetLocation)
	{
		m_strLocation = strLocation;
	}
	m_bHaveSetLocation = true;
}

//����һЩû����ȷ����ķǱ�׼��http��Ӧͷ
//��ʽ��:x-token: AAA\r\nx-auth: BBB\r\n
void HttpResponse::SetHeader(const std::string& strFieldAndValue)
{
	m_strOtherFieldValue = strFieldAndValue;
}


// ���������Ƿ񱣳֣�bKeepAlive=1����:Keep-Alive��bKeepAlive=0������:Close
// ע��:�ú����ڲ���У�飬ֻ�������һ��
void HttpResponse::SetKeepAlive(bool bKeepAlive)
{
	if (!m_bHaveSetKeepAlive)
	{
		m_bKeepAlive = bKeepAlive;
	}
	m_bHaveSetKeepAlive = true;
}
// �Ƿ���Ҫ���ֳ�����
bool HttpResponse::IsKeepAlive() const
{
	return m_bKeepAlive;
}

// ���û�Ӧ���İ���
// ע��:������SetStatusCode&SetContentType&SetKeepAlive������OK���Ժ�ſ��Ե���
// ע��:�ú����ڲ���У�飬ֻ�������һ��
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
		// �����body��ֱ�Ӹ�ʽ�������bodyû������׼��statuscodereason
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

// ���ϲ�����ʽ���õ������Ļ�Ӧ���ģ�ͷ+�壩���ý���
// ʹ�øú���ʱSetStatusCode&SetContentType&SetKeepAlive�Կ���ͬʱ���ã���SetResponseBodyһ�����ܵ���
void HttpResponse::SetHttpResponse(std::string& strResponse)
{
	m_strResponse.swap(strResponse);
}

// ���ظ�ʽ���õ�httpresponse�ַ���-ע��:������SetResponseBody����OK���Ժ�ſ��Ե���
const std::string& HttpResponse::GetHttpResponse() const
{
	return m_strResponse;
}

// ����http��Ӧ���ܳ���(ͷ+��)-ע��:������SetResponseBody����OK���Ժ�ſ��Ե���
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
		// �������¼��ص�(connect,disconnect)
		m_server.setConnectionCallback(std::bind(&HttpNetServerImpl::onConnection, this, _1));
		// ���յ������ݻص�
		m_server.setMessageCallback(std::bind(&HttpNetServerImpl::onMessage, this, _1, _2, _3, std::placeholders::_4));
		// ��д������ɻص�
		m_server.setWriteCompleteCallback(std::bind(&HttpNetServerImpl::onWriteOk, this, _1, _2));
		// ��д�����쳣��ʧ�ܻص�
		m_server.setWriteErrCallback(std::bind(&HttpNetServerImpl::onWriteErr, this, _1, _2, _3));
		// ���߳������ɹ��ص�
		m_server.setThreadInitCallback(std::bind(&HttpNetServerImpl::onThreadInit, this, _1));
		// ���ù����߳���
		m_server.setThreadNum(threadnum);
		// ����server
		m_loop->runInLoop(std::bind(&HttpNetServerImpl::start, this));
	}

	// ���ÿ������߿�ʱ��
	void ResetIdleConnTime(int idleconntime)
	{
		m_nIdleConnTime = idleconntime;
	}

  	// ��Ӧ
	int OnSendMsg(const TcpConnectionPtr& conn, const boost::any& params, const HttpResponse& rsp, int timeout)
	{
		uint64_t request_id = MakeSnowId(m_ptrCtrlParams->g_unique_node_id, m_ptrCtrlParams->g_nSequnceId);
		tInnerParams inner_params(request_id, params);
		boost::any inner_any(inner_params);

		if (m_ptrCtrlParams->g_bEnableSendLog_http)
		{
			LOG_INFO << "[HTTP][SENDING]"
				<< "[" << request_id << "]"
				<< "[" << conn->getConnuuid() << "]"
				<< "[" << conn->peerAddress().toIpPort() << "]"
				<< ":" << rsp.GetHttpResponse();
		}

		if (!rsp.IsKeepAlive()) conn->setNeedCloseFlag();
		int nSendRet = conn->send(rsp.GetHttpResponse(), inner_any, timeout);

		if (m_ptrCtrlParams->g_bEnableSendLog_http && 0 != nSendRet)
		{
			LOG_WARN << "[HTTP][SENDERR]"
				<< "[" << request_id << "]"
				<< "[" << conn->getConnuuid() << "]"
				<< "[" << conn->peerAddress().toIpPort() << "]"
				<< ":" << "SEND ERROR,ERCODE:" << nSendRet;
		}

		return nSendRet;
	}

	// ǿ�ƹر�����
	void ForceCloseConnection(const TcpConnectionPtr& conn)
	{
		conn->shutdown();
		//conn->forceCloseWithDelay(1.0);  // > round trip of the whole Internet.
		conn->forceClose();
	}

	// ����http�Ự����Ϣ(�봫session�������ָ�룬����ֻ�������һ��&session�������п��ƶ��̻߳�������)
	void SetHttpSessionInfo(const TcpConnectionPtr& conn, const boost::any& sessioninfo)
	{
		conn->setSessionData(sessioninfo);
	}

	// ���ø�ˮλֵ
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
			uint64_t conn_uuid = MakeSnowId(m_ptrCtrlParams->g_unique_node_id, m_ptrCtrlParams->g_nSequnceId);
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
						//��Ϊ��Чpos
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

			// �������������
			if (m_ptrCtrlParams->g_nCurrConnNum_Http.incrementAndGet() > m_ptrCtrlParams->g_nMaxConnNum_Http)
		    {
		      	conn->shutdown();
		      	//conn->forceCloseWithDelay(1.0);  // > round trip of the whole Internet.
		      	conn->forceClose();
				LOG_WARN << "HttpServer reached the maxconnnum:" << m_ptrCtrlParams->g_nMaxConnNum_Http;
				return;
		    }

			// ����һ��httprequest���󣬱��浽���ӵ���������http�������
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
					// �ص��ɹ�,���账��
				}
				else
				{
					// ��������ֵ���޶���,�ݲ�����
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
			bool bParseOver = false;
			// ִ�н���
			int nParseRet = reqptr->ParseHttpRequest(strRecvMsg.data(), strRecvMsg.size(), m_ptrCtrlParams->g_nMaxRecvBuf_Http, bParseOver);
			// �������ɹ�
			if (0 == nParseRet)
			{
				// �������꣬�������꣬�ص����ϲ�
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
							// �ص��ɹ�,���账��
						}
						else
						{
							// ��������ֵ���޶���,�ݲ�����
						}
					}

					// ��buf�е�ָ���λ
					buf->retrieveAll();

					// ��λ������
					reqptr->ResetParser();

					// ���Ʊ�־��λ
					reqptr->ResetSomeCtrlFlag();
				}
				else
				{
					// ������Expect: 100-Continue
					// ����û�лع�,����Ӧ100-Continue
					if (!reqptr->HasRsp100Continue() && reqptr->b100ContinueInHeader())
					{
						boost::any params;
						uint64_t request_id = MakeSnowId(m_ptrCtrlParams->g_unique_node_id, m_ptrCtrlParams->g_nSequnceId);
						tInnerParams inner_params(request_id, params);
						boost::any inner_any(inner_params);

						if (m_ptrCtrlParams->g_bEnableSendLog_http)
						{
							LOG_INFO << "[HTTP][SENDING]"
								<< "[" << request_id << "]"
								<< "[" << conn->getConnuuid() << "]"
								<< "[" << conn->peerAddress().toIpPort() << "]"
								<< ":" << "100-continue";
						}

						//���ѻ�Ӧ��100continue��־
						reqptr->SetHasRsp100Continue();
						// ��Ӧ100-continue
						int nSendRet = conn->send("100-continue", inner_any);

						if (m_ptrCtrlParams->g_bEnableSendLog_http && 0 != nSendRet)
						{
							LOG_WARN << "[HTTP][SENDERR]"
								<< "[" << request_id << "]"
								<< "[" << conn->getConnuuid() << "]"
								<< "[" << conn->peerAddress().toIpPort() << "]"
								<< ":" << "SEND ERROR,ERCODE:" << nSendRet;
						}
					}

					// ��buf�е�ָ���λ
					buf->retrieveAll();

					// ��λ������
					reqptr->ResetParser();
				}
			}
			else
			{
				std::string strBadRequest = "HTTP/1.1 400 Bad Request\r\n\r\n";
				std::string strErrMsg = "http parse fail";
				if (1 == nParseRet)
				{
					strErrMsg = "http parse fail";
				}
				else if (2 == nParseRet)
				{
					strErrMsg = "http request overlength";
				}

				LOG_WARN << "[HTTP][RECVERR]"
					<< "[" << conn->getConnuuid() << "]"
					<< ":" << strErrMsg;

				// ��buf�е�ָ���ƹ�λ
				buf->retrieveAll();

				// ���Ʊ�־��λ
				reqptr->ResetSomeCtrlFlag();

				// ��λ������
				reqptr->ResetParser();

				// ����������Ҫ�رձ�־
				conn->setNeedCloseFlag();

				boost::any params;
				uint64_t request_id = MakeSnowId(m_ptrCtrlParams->g_unique_node_id, m_ptrCtrlParams->g_nSequnceId);
				tInnerParams inner_params(request_id, params);
				boost::any inner_any(inner_params);

				if (m_ptrCtrlParams->g_bEnableSendLog_http)
				{
					LOG_WARN << "[HTTP][SENDING]"
						<< "[" << request_id << "]"
						<< "[" << conn->getConnuuid() << "]"
						<< "[" << conn->peerAddress().toIpPort() << "]"
						<< ":" << strBadRequest << strErrMsg;
				}

				// ����ʧ�ܣ�ֱ������ʧ�ܵ�response,��:400
				int nSendRet = conn->send(strBadRequest + strErrMsg, inner_any);

				if (m_ptrCtrlParams->g_bEnableSendLog_http && 0 != nSendRet)
				{
					LOG_WARN << "[HTTP][SENDERR]"
						<< "[" << request_id << "]"
						<< "[" << conn->getConnuuid() << "]"
						<< "[" << conn->peerAddress().toIpPort() << "]"
						<< ":" << "SEND ERROR,ERCODE:" << nSendRet;
				}
			}
		}
	}

	void onMessage(const TcpConnectionPtr& conn, Buffer* buf, size_t len, Timestamp time)
	{
		if (!conn) return;

		// write logs
		if (m_ptrCtrlParams->g_bEnableRecvLog_http)
		{
			uint64_t request_id = MakeSnowId(m_ptrCtrlParams->g_unique_node_id, m_ptrCtrlParams->g_nSequnceId);
			const char* pRecv = buf->reverse_peek(len);
			LOG_INFO << "[HTTP][RECVOK ]"
				<< "[" << request_id << "]"
				<< "[" << conn->getConnuuid() << "]"
				<< "[" << conn->peerAddress().toIpPort() << "]"
				<< ":" << (pRecv ? pRecv : "RECV NULL");
		}

		m_ptrCtrlParams->g_nTotalReqNum_Http.increment();

		UpdateConnList(conn, false);

		DealHttpRequest(conn, buf, time);
	}

	void onWriteOk(const TcpConnectionPtr& conn, const boost::any& params)
	{
		if (!conn) return;

		tInnerParams inner_params(boost::any_cast<tInnerParams>(params));

		if (m_ptrCtrlParams->g_bEnableRecvLog_http)
		{
			LOG_INFO << "[HTTP][SENDOK ]"
				<< "[" << inner_params.request_id << "]"
				<< "[" << conn->getConnuuid() << "]"
				<< "[" << conn->peerAddress().toIpPort() << "]";
		}

		// �����ܴ���������
		m_ptrCtrlParams->g_nTotalWaitSendNum_http.decrement();
		
		int nRet = 0;
		if (m_pfunc_on_sendok)
		{
			WeakTcpConnectionPtr weakconn(conn);
			nRet = m_pfunc_on_sendok(m_pInvoker, conn->getConnuuid(), weakconn, conn->getSessionData(), inner_params.params);
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

		tInnerParams inner_params(boost::any_cast<tInnerParams>(params));

		if (m_ptrCtrlParams->g_bEnableRecvLog_http)
		{
			LOG_ERROR << "[HTTP][SENDERR]"
				<< "[" << inner_params.request_id << "]"
				<< "[" << conn->getConnuuid() << "]"
				<< "[" << conn->peerAddress().toIpPort() << "]"
				<< ":" << "SEND ERROR,ERCODE:" << errCode;
		}

		// �����ܴ���������
		m_ptrCtrlParams->g_nTotalWaitSendNum_http.decrement();

		int nRet = 0;
		if (m_pfunc_on_senderr)
		{
			WeakTcpConnectionPtr weakconn(conn);
			nRet = m_pfunc_on_senderr(m_pInvoker, conn->getConnuuid(), weakconn, conn->getSessionData(), inner_params.params, errCode);
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

	// ������ʱ��
	void startTimer()
	{
		EventLoop* loop = m_server.threadPool()->getNextLoop();
		loop->runInLoop(std::bind(&HttpNetServerImpl::InitConnList, this));
		loop->runEvery(1.0, std::bind(&HttpNetServerImpl::onCheckConnTimeOut, this));
	}

    // ��ʼ�����Ӷ���
	void InitConnList()
	{
		ThreadLocalSingleton<WeakConnectionList>::instance();
	}

	// ��ʱ��,��ʱ��ⳬʱ
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
					// ��ʱ
					if (nTmDiff > m_nIdleConnTime)
					{
						if (conn->connected())
						{
							LOG_INFO<<"IP:["<<conn->peerAddress().toIpPort()<<"] CONNUUID:["<<conn->getConnuuid()<<"] "<<nTmDiff<<" s NO ACTIVE,KICK IT OUT.\n";
							// �߳�
							conn->shutdown();
							//conn->forceCloseWithDelay(1.0);  // > round trip of the whole Internet.
							conn->forceClose();
						}
					}
					// ʱ��������
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
				// ������ʧЧ
				connlist.erase(it++);
			}
		}
	}

private:
	void* m_pInvoker;
	char m_index;
	EventLoop* m_loop;
	TcpServer m_server;
	int m_nIdleConnTime;		// �����ӳ�ʱ�߳�ʱ��
	size_t m_nHighWaterMark;	// ��ˮλֵ
	
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
// TCP����
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// ����TCP����
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
		
	// �ر�TCP����
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
	
	// ��ȡ��ǰTCP��������
	int GetCurrTcpConnNum()
	{
		net_ctrl_params_ptr pCtrlParams = m_ptrCtrlParams;
		if (pCtrlParams) return pCtrlParams->g_nCurrConnNum_Tcp.get();
		return 0;
	}
	// ��ȡ��ǰTCP��������
	int64_t GetTotalTcpReqNum()
	{
		net_ctrl_params_ptr pCtrlParams = m_ptrCtrlParams;
		if (pCtrlParams) return pCtrlParams->g_nTotalReqNum_Tcp.get();
		return 0;
	}
	
	//��ȡ��ǰTCP�ܵĵȴ����͵����ݰ�����
	int64_t GetTotalTcpWaitSendNum()
	{
		net_ctrl_params_ptr pCtrlParams = m_ptrCtrlParams;
		if (pCtrlParams) return pCtrlParams->g_nTotalWaitSendNum_tcp.get();
		return 0;
	}
	// ������Ϣ
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
					ret = m_TcpNetServerImpls[node->GetIndex()]->OnSendMsg(pConn, params, szMsg, nMsgLen, timeout, bKeepAlive);
				}
			}
			// ���Ӵ������ܵļ���
			if (0 == ret) pCtrlParams->g_nTotalWaitSendNum_tcp.increment();
		}
		return ret;
	}
	// ǿ�ƹر�TCP����
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
	//��ͣ��ǰTCP���ӵ����ݽ���
	void SuspendTcpConnRecv(uint64_t conn_uuid, const boost::any& conn, int delay)
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
					m_TcpNetServerImpls[node->GetIndex()]->SuspendConnRecv(pConn, delay);
				}
			}
		}
	}
	//�ָ���ǰTCP���ӵ����ݽ���
	void ResumeTcpConnRecv(uint64_t conn_uuid, const boost::any& conn)
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
					m_TcpNetServerImpls[node->GetIndex()]->ResumeConnRecv(pConn);
				}
			}
		}
	}
	// ����TCP�������߿�ʱ��
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
	// ����TCP��֧�ֵ����������
	void ResetTcpMaxConnNum(int maxconnnum)
	{	
		net_ctrl_params_ptr pCtrlParams = m_ptrCtrlParams;
		if (pCtrlParams) pCtrlParams->g_nMaxConnNum_Tcp = maxconnnum;
	}

	// ����tcp�Ự����Ϣ(�봫session�������ָ�룬����ֻ�������һ��&session�������п��ƶ��̻߳�������)
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

	// ���ø�ˮλֵ (thread safe)
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
// HTTP����
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// ����HTTP����
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
		
	// �ر�HTTP����
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

	// ��ȡ��ǰHTTP��������
	int GetCurrHttpConnNum()
	{
		net_ctrl_params_ptr pCtrlParams = m_ptrCtrlParams;
		if (pCtrlParams) return pCtrlParams->g_nCurrConnNum_Http.get();
		return 0;
	}
	// ��ȡ��ǰHTTP��������
	int64_t GetTotalHttpReqNum()
	{
		net_ctrl_params_ptr pCtrlParams = m_ptrCtrlParams;
		if (pCtrlParams) return pCtrlParams->g_nTotalReqNum_Http.get();
		return 0;
	}
	//��ȡ��ǰHTTP�ܵĵȴ����͵����ݰ�����
	int64_t GetTotalHttpWaitSendNum()
	{
		net_ctrl_params_ptr pCtrlParams = m_ptrCtrlParams;
		if (pCtrlParams) return pCtrlParams->g_nTotalWaitSendNum_http.get();
		return 0;
	}
	// ������Ϣ
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
					ret = m_HttpNetServerImpls[node->GetIndex()]->OnSendMsg(pConn, params, rsp, timeout);
				}
			}
			// ���Ӵ������ܵļ���
			if (0 == ret) pCtrlParams->g_nTotalWaitSendNum_http.increment();
		}
		return ret;
	}
	// ǿ�ƹر�HTTP����
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
	// ����HTTP�������߿�ʱ��
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

	// ����HTTP��֧�ֵ����������
	void ResetHttpMaxConnNum(int maxconnnum)
	{
		net_ctrl_params_ptr pCtrlParams = m_ptrCtrlParams;
		if (pCtrlParams) pCtrlParams->g_nMaxConnNum_Http = maxconnnum;
	}

	
	// ����http�Ự����Ϣ(�봫session�������ָ�룬����ֻ�������һ��&session�������п��ƶ��̻߳�������)
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

	// ���ø�ˮλֵ (thread safe)
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
	// ���̶߳�ʱɨ��
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
	// ���̶߳�ʱɨ��
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
	// ------TCP ����˵�loop
	EventLoop* m_pTcpLoop;
	EventLoopThreadPool* m_pTcpEventLoopThreadPool;
	std::vector<TcpNetServerImpl*> m_TcpNetServerImpls;

	// ------HTTP ����˵�loop
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

	//��ʱȡ��
	mkdir("./netlogs", 0755);
	if (!g_logFile) g_logFile.reset(new mwnet_mt::LogFile("./netlogs/", "netserverlog", 1024*1024*100, true, 0, 1));
  	mwnet_mt::Logger::setOutput(outputFunc);
  	mwnet_mt::Logger::setFlush(flushFunc);

	//Ĭ��������־
	ptrParams->g_bEnableRecvLog_http = true;
	ptrParams->g_bEnableSendLog_http = true;
	ptrParams->g_bEnableRecvLog_tcp = true;
	ptrParams->g_bEnableSendLog_tcp = true;
}

//����nodeid(1~8000��ȡֵ)
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

	//��ʱȡ��
	mkdir("./netlogs", 0755);
	if (!g_logFile) g_logFile.reset(new mwnet_mt::LogFile("./netlogs/", "netserverlog", 1024*1024*100, true, 0, 1));
  	mwnet_mt::Logger::setOutput(outputFunc);
  	mwnet_mt::Logger::setFlush(flushFunc);

	//Ĭ��������־
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

// ������رս���/������־
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
// ����TCP����
void NetServer::StartTcpServer(void* pInvoker,uint16_t listenport,int threadnum,int idleconntime,int maxconnnum,bool bReusePort,
	pfunc_on_connection pOnConnection, pfunc_on_readmsg_tcp pOnReadMsg, pfunc_on_sendok pOnSendOk, 
	pfunc_on_senderr pOnSendErr, pfunc_on_highwatermark pOnHighWaterMark,
	size_t nDefRecvBuf, size_t nMaxRecvBuf, size_t nMaxSendQue)
{
	m_NetServerCtrl->StartTcpServer(pInvoker,listenport,threadnum,idleconntime,maxconnnum,bReusePort,pOnConnection,
		pOnReadMsg,pOnSendOk, pOnSendErr,pOnHighWaterMark,nDefRecvBuf,nMaxRecvBuf,nMaxSendQue);
}
// �ر�TCP����
void NetServer::StopTcpServer()
{
	if (m_NetServerCtrl) m_NetServerCtrl->StopTcpServer();
}
// ��ȡ��ǰTCP��������
int NetServer::GetCurrTcpConnNum()
{
	if (m_NetServerCtrl) return m_NetServerCtrl->GetCurrTcpConnNum();
	return 0;
}
// ��ȡ��ǰTCP��������
int64_t NetServer::GetTotalTcpReqNum()
{
	if (m_NetServerCtrl) return m_NetServerCtrl->GetTotalTcpReqNum();
	return 0;
}
//��ȡ��ǰTCP�ܵĵȴ����͵����ݰ�����
int64_t NetServer::GetTotalTcpWaitSendNum()
{
	if (m_NetServerCtrl) return m_NetServerCtrl->GetTotalTcpWaitSendNum();
	return 0;
}

// ������Ϣ
int NetServer::SendMsgWithTcpServer(uint64_t conn_uuid, const boost::any& conn, const boost::any& params, const char* szMsg, size_t nMsgLen, int timeout, bool bKeepAlive)
{
	if (m_NetServerCtrl) return m_NetServerCtrl->SendMsgWithTcpServer(conn_uuid, conn, params, szMsg, nMsgLen, timeout, bKeepAlive);
	return 1;
}
//ǿ�ƹر�TCP����
void NetServer::ForceCloseTcpConnection(uint64_t conn_uuid, const boost::any& conn)
{
	if (m_NetServerCtrl) m_NetServerCtrl->ForceCloseTcpConnection(conn_uuid, conn);
}
//��ͣ��ǰTCP���ӵ����ݽ���
void NetServer::SuspendTcpConnRecv(uint64_t conn_uuid, const boost::any& conn, int delay)
{
	if (m_NetServerCtrl) m_NetServerCtrl->SuspendTcpConnRecv(conn_uuid, conn, delay);
}
//�ָ���ǰTCP���ӵ����ݽ���
void  NetServer::ResumeTcpConnRecv(uint64_t conn_uuid, const boost::any& conn)
{
	if (m_NetServerCtrl) m_NetServerCtrl->ResumeTcpConnRecv(conn_uuid, conn);
}
// ����TCP�������߿�ʱ��
void NetServer::ResetTcpIdleConnTime(int idleconntime)
{
	if (m_NetServerCtrl) m_NetServerCtrl->ResetTcpIdleConnTime(idleconntime);
}

// ����TCP��֧�ֵ����������
void NetServer::ResetTcpMaxConnNum(int maxconnnum)
{
	if (m_NetServerCtrl) m_NetServerCtrl->ResetTcpMaxConnNum(maxconnnum);
}

//����tcp�Ự����Ϣ(�봫session�������ָ�룬����ֻ�������һ��&session�������п��ƶ��̻߳�������)
void NetServer::SetTcpSessionInfo(uint64_t conn_uuid, const boost::any& conn, const boost::any& sessioninfo)
{
	if (m_NetServerCtrl) m_NetServerCtrl->SetTcpSessionInfo(conn_uuid, conn, sessioninfo);
}

//���ø�ˮλֵ (thread safe)
void NetServer::SetTcpHighWaterMark(uint64_t conn_uuid, const boost::any& conn, size_t highWaterMark)
{
	if (m_NetServerCtrl) m_NetServerCtrl->SetTcpHighWaterMark(conn_uuid, conn, highWaterMark);
}

//-------------------------------------------TcpServer End----------------------------------------------------


//-------------------------------------------HttpServer Begin-------------------------------------------------
// ����HTTP����
void NetServer::StartHttpServer(void* pInvoker,uint16_t listenport,int threadnum,int idleconntime,int maxconnnum,bool bReusePort,
	pfunc_on_connection pOnConnection, pfunc_on_readmsg_http pOnReadMsg, pfunc_on_sendok pOnSendOk, 
	pfunc_on_senderr pOnSendErr,pfunc_on_highwatermark pOnHighWaterMark, 
	size_t nDefRecvBuf, size_t nMaxRecvBuf, size_t nMaxSendQue)
{
	if (m_NetServerCtrl) 
	m_NetServerCtrl->StartHttpServer(pInvoker,listenport,threadnum,idleconntime,maxconnnum,bReusePort,pOnConnection,
		pOnReadMsg,pOnSendOk,pOnSendErr,pOnHighWaterMark,nDefRecvBuf,nMaxRecvBuf,nMaxSendQue);
}
// �ر�HTTP����
void NetServer::StopHttpServer()
{
	if (m_NetServerCtrl) m_NetServerCtrl->StopHttpServer();
}
// ��ȡ��ǰHTTP��������
int NetServer::GetCurrHttpConnNum()
{
	if (m_NetServerCtrl) return m_NetServerCtrl->GetCurrHttpConnNum();
	return 0;
}
// ��ȡ��ǰHTTP��������
int64_t NetServer::GetTotalHttpReqNum()
{
	if (m_NetServerCtrl) return m_NetServerCtrl->GetTotalHttpReqNum();
	return 0;
}
//��ȡ��ǰHTTP�ܵĵȴ����͵����ݰ�����
int64_t NetServer::GetTotalHttpWaitSendNum()
{
	if (m_NetServerCtrl) return m_NetServerCtrl->GetTotalHttpWaitSendNum();
	return 0;
}

// ����HTTP��Ӧ��Ϣ
int NetServer::SendHttpResponse(uint64_t conn_uuid, const boost::any& conn, const boost::any& params, const HttpResponse& rsp, int timeout)
{
	if (m_NetServerCtrl) return m_NetServerCtrl->SendHttpResponse(conn_uuid, conn, params, rsp, timeout);
	return 1;
}
//ǿ�ƹر�HTTP����
void NetServer::ForceCloseHttpConnection(uint64_t conn_uuid, const boost::any& conn)
{
	if (m_NetServerCtrl) m_NetServerCtrl->ForceCloseHttpConnection(conn_uuid, conn);
}

// ����HTTP�������߿�ʱ��
void NetServer::ResetHttpIdleConnTime(int idleconntime)
{
	if (m_NetServerCtrl) m_NetServerCtrl->ResetHttpIdleConnTime(idleconntime);
}

// ����HTTP��֧�ֵ����������
void NetServer::ResetHttpMaxConnNum(int maxconnnum)
{
	if (m_NetServerCtrl) m_NetServerCtrl->ResetHttpMaxConnNum(maxconnnum);
}

//����http�Ự����Ϣ(�봫session�������ָ�룬����ֻ�������һ��&session�������п��ƶ��̻߳�������)
void NetServer::SetHttpSessionInfo(uint64_t conn_uuid, const boost::any& conn, const boost::any& sessioninfo)
{
	if (m_NetServerCtrl) m_NetServerCtrl->SetHttpSessionInfo(conn_uuid, conn, sessioninfo);
}

//���ø�ˮλֵ (thread safe)
void NetServer::SetHttpHighWaterMark(uint64_t conn_uuid, const boost::any& conn, size_t highWaterMark)
{
	if (m_NetServerCtrl) m_NetServerCtrl->SetHttpHighWaterMark(conn_uuid, conn, highWaterMark);
}

//-------------------------------------------HttpServer End----------------------------------------------------

}
}
