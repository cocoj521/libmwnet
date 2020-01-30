#include "TcpClient.h"
#include <mwnet_mt/base/Thread.h>
#include <mwnet_mt/base/Atomic.h>
#include <mwnet_mt/base/Timestamp.h>
#include <mwnet_mt/net/TcpClient.h>
#include <mwnet_mt/net/EventLoopThreadPool.h>
#include <mwnet_mt/net/EventLoop.h>
#include <mwnet_mt/net/EventLoopThread.h>
#include <mwnet_mt/net/InetAddress.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <memory>  
#include <iostream>

using namespace mwnet_mt;
using namespace mwnet_mt::net;

namespace MWNET_MT
{
namespace TCPCLIENT
{
	// ��������
	bool domain_resolver(const std::string& strDomain, std::string& strIp)
	{
		bool bRet = true;
		struct hostent  *pHostInfo = NULL;
		struct in_addr addr;

		do
		{
			//�����������ֺ͵�ַ��Ϣ��hostent�ṹָ��
			pHostInfo = gethostbyname(strDomain.c_str());

			if (NULL == pHostInfo)
			{
				//IP����ʧ�ܻ���Ч��IP,�����粻ͨ
				if (inet_pton(AF_INET, strDomain.c_str(), &addr) <= 0)
				{
					;
				}

				pHostInfo = gethostbyaddr(&addr, sizeof(addr), AF_INET);
				if (NULL == pHostInfo)
				{
					bRet = false;
					break;
				}
			}

			if (NULL == pHostInfo->h_addr_list || NULL == pHostInfo->h_addr_list[0])
			{
				bRet = false;
				break;
			}
			struct in_addr* p = reinterpret_cast<in_addr*>(pHostInfo->h_addr_list[0]);
			strIp = inet_ntoa(*p);
		} while (0);

		return bRet;
	}

	// ����ѩ��,1���ڲ��ظ�
	AtomicInt64 g_nSequnceId;
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

		nYear = tm_time.tm_year + 1900;
		nYear = nYear % 100;
		nMonth = tm_time.tm_mon + 1;
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
		j |= static_cast<int64_t>(nNo & 0xFFFFFF);	//seqid,0~0xFFFFFF

		return j;
	}

	typedef std::weak_ptr<TcpConnection> WeakTcpConnectionPtr;
	class xTcpClient;
	typedef std::shared_ptr<xTcpClient> xTcpClientPtr;
	typedef std::shared_ptr<TcpClient> TcpClientPtr;

	class xTcpClient
		: public std::enable_shared_from_this<xTcpClient>
	{
	public:
		xTcpClient(void* pInvoker, const ConnBaseInfoPtr& pConnBaseInfo,
			EventLoop* loop, const InetAddress& svrAddr,
			pfunc_on_connection pOnConnection,
			pfunc_on_readmsg_tcp pOnReadMsg,
			pfunc_on_sendok pOnSendOk,
			pfunc_on_senderr pOnSendErr,
			pfunc_on_highwatermark pOnHighWaterMark,
			size_t nDefRecvBuf, size_t nMaxRecvBuf, size_t nMaxSendQue)
			:
			m_pInvoker(pInvoker),
			loop_(loop),
			conn_baseinfo_(pConnBaseInfo),
			highWaterMark_(nMaxSendQue),
			maxRecvBuf_(nMaxRecvBuf),
			m_pfunc_on_connection(pOnConnection),
			m_pfunc_on_readmsg_tcp(pOnReadMsg),
			m_pfunc_on_sendok(pOnSendOk),
			m_pfunc_on_senderr(pOnSendErr),
			m_pfunc_on_highwatermark(pOnHighWaterMark),
			client_(new TcpClient(loop, svrAddr, "mTcpCli", nDefRecvBuf, nMaxRecvBuf, nMaxSendQue))
		{
			// �������¼��ص�(connect,disconnect)
			client_->setConnectionCallback(std::bind(&xTcpClient::onConnection, this, _1, _2));
			// ���յ������ݻص�
			client_->setMessageCallback(std::bind(&xTcpClient::onMessage, this, _1, _2, _3, std::placeholders::_4));
			// ��д������ɻص�
			client_->setWriteCompleteCallback(std::bind(&xTcpClient::onWriteOk, this, _1, _2));
			// ��д�����쳣��ʧ�ܻص�
			client_->setWriteErrCallback(std::bind(&xTcpClient::onWriteErr, this, _1, _2, _3));
		}

		~xTcpClient()
		{
			//LOG_INFO << "xTcpClient::~xTcpClient()";
		}
		
		void Connect()
		{
			self_ = shared_from_this();
			client_->connect(conn_baseinfo_->conn_timeout);
		}

		void DisConnect()
		{
			client_->disconnect();
		}

		void Stop()
		{
			client_->stop();
		}

		// ���ø�ˮλֵ
		void SetHighWaterMark(size_t highwatermark)
		{
			highWaterMark_ = highwatermark;
		}

		//��ͣ��ǰTCP���ӵ����ݽ���
		void SuspendTcpConnRecv(int delay)
		{
			TcpConnectionPtr pConn(weak_conn_.lock());
			if (pConn)
			{
				pConn->suspendRecv(delay);
			}
		}

		//�ָ���ǰTCP���ӵ����ݽ���
		void ResumeTcpConnRecv()
		{
			TcpConnectionPtr pConn(weak_conn_.lock());
			if (pConn)
			{
				pConn->resumeRecv();
			}
		}

		int SendTcpRequest(const boost::any& params, const char* szMsg, size_t nMsgLen, int timeout_ms, bool bKeepAlive)
		{
			int nRet = 1;
			TcpConnectionPtr pConn(weak_conn_.lock());
			if (pConn)
			{
				if (!bKeepAlive) pConn->setNeedCloseFlag();
				nRet = pConn->send(szMsg, static_cast<int>(nMsgLen), params, timeout_ms);
			}
			return nRet;
		}
		
		void tie(const boost::any& context)
		{
			context_ = context;
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
			return client_->getInetAddress().toIpPort().c_str();
		}
	private:
		void onConnection(const TcpConnectionPtr& conn, const std::string& errDesc)
		{
			// ����ʧ��
			if (!conn) 
			{
				if (m_pfunc_on_connection)
				{
					conn_baseinfo_->conn_end_tm_ms = Timestamp::GetCurrentTimeMs();
					conn_baseinfo_->conn_status = CONNSTATUS_CONNECTFAIL;
					conn_baseinfo_->conn_status_desc = std::string("connect fail") + ":" + errDesc;
					m_pfunc_on_connection(m_pInvoker, conn_baseinfo_, false, client_->getInetAddress().toIpPort().c_str());
				}

				if (self_) self_.reset();
				return;
			}
			// ���ӳɹ�
			if (conn->connected())
			{
				weak_conn_ = conn;				
				conn->setConnuuid(conn_baseinfo_->conn_uuid);		
				conn->setHighWaterMarkCallback(std::bind(&xTcpClient::onHighWater, shared_from_this(), _1, _2), highWaterMark_);
							
				if (m_pfunc_on_connection)
				{
					conn_baseinfo_->conn_end_tm_ms = Timestamp::GetCurrentTimeMs();
					conn_baseinfo_->conn_status = CONNSTATUS_CONNECTED;
					conn_baseinfo_->conn_status_desc = std::string("connect success") + ":" + errDesc;
					int nRet = m_pfunc_on_connection(m_pInvoker, conn_baseinfo_, true, client_->getInetAddress().toIpPort().c_str());
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
			//���ӶϿ�
			else if (conn->disconnected())
			{
				if (m_pfunc_on_connection)
				{
					conn_baseinfo_->conn_status = CONNSTATUS_DISCONNECT;
					conn_baseinfo_->conn_status_desc = std::string("disconnect") + ":" + errDesc;
					m_pfunc_on_connection(m_pInvoker, conn_baseinfo_, false, client_->getInetAddress().toIpPort().c_str());
				}
				if (self_) self_.reset();
			}
		}

		void onMessage(const TcpConnectionPtr& conn, Buffer* buf, size_t len, Timestamp time)
		{
			if (!conn) return;

			//write logs tcpclient��û������־��,�ݲ�������־
			/*
			LOG_INFO << "[HTTP][RECV]["
				<< conn->getConnuuid() << "]["
				<< conn->peerAddress().toIpPort() << "]:"
				<< StringUtil::BytesToHexString(buf->reverse_peek(len), len);
			*/
			if (m_pfunc_on_readmsg_tcp)
			{
				int nReadedLen = 0;
				StringPiece RecvMsg = buf->toStringPiece();
			
				int nRet = m_pfunc_on_readmsg_tcp(m_pInvoker, conn_baseinfo_, RecvMsg.data(), RecvMsg.size(), nReadedLen);
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
					if (buf->readableBytes() >= maxRecvBuf_)
					{
						nRet = -1;
						// LOG_WARN<<"[TCP][RECV]["<<conn->getConnuuid()<<"]\nillegal data size:"<<buf->readableBytes()<<",maybe too long\n";
					}
				}
				// �жϷ���ֵ��������ͬ�Ĵ���
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
				nRet = m_pfunc_on_sendok(m_pInvoker, conn_baseinfo_, params);
			}
		
			if (nRet < 0 || conn->bNeedClose())
			{
				conn->shutdown();
				conn->forceClose();
			}
		}

		void onWriteErr(const TcpConnectionPtr& conn, const boost::any& params, int errCode)
		{
			if (!conn) return;

			int nRet = 0;
			if (m_pfunc_on_sendok)
			{
				nRet = m_pfunc_on_senderr(m_pInvoker, conn_baseinfo_, params, errCode);
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

			int nRet = 0;
			if (m_pfunc_on_highwatermark)
			{
				nRet = m_pfunc_on_highwatermark(m_pInvoker, conn_baseinfo_, highWaterMark);
			}

			if (nRet < 0 || conn->bNeedClose())
			{
				conn->shutdown();
				conn->forceClose();
			}
		}
		
	private:
		// �ϲ���õ���ָ��
		void* m_pInvoker;
		EventLoop* loop_;
		ConnBaseInfoPtr conn_baseinfo_;
		size_t highWaterMark_;
		size_t maxRecvBuf_;
		pfunc_on_connection m_pfunc_on_connection;
		pfunc_on_readmsg_tcp m_pfunc_on_readmsg_tcp;
		pfunc_on_sendok m_pfunc_on_sendok;
		pfunc_on_senderr m_pfunc_on_senderr;
		pfunc_on_highwatermark m_pfunc_on_highwatermark;
		WeakTcpConnectionPtr weak_conn_;
		TcpClientPtr client_;
		xTcpClientPtr self_;
		boost::any context_;
	};


////////////////////////////////////////////////////////////////////////////////////////////////
	typedef std::shared_ptr<EventLoopThread> EventLoopThreadPtr;
	typedef std::shared_ptr<EventLoopThreadPool> EventLoopThreadPoolPtr;

	mTcpClient::mTcpClient()
	{
		exit_ = true;
	}

	mTcpClient::~mTcpClient()
	{	
	}

	// ��ʼ��
	bool mTcpClient::InitTcpClient(void* pInvoker,										
									pfunc_on_connection pOnConnection,					
									pfunc_on_readmsg_tcp pOnReadMsg,					
									pfunc_on_sendok pOnSendOk,							
									pfunc_on_senderr pOnSendErr,						
									pfunc_on_highwatermark pOnHighWaterMark ,		
									uint16_t threadnum,								
									size_t nDefRecvBuf,					
									size_t nMaxRecvBuf,					
									size_t nMaxSendQue					
									)
	{
		m_pInvoker = pInvoker;
		m_pfunc_on_connection = pOnConnection;
		m_pfunc_on_readmsg_tcp = pOnReadMsg;
		m_pfunc_on_sendok = pOnSendOk;
		m_pfunc_on_senderr = pOnSendErr;
		m_pfunc_on_highwatermark = pOnHighWaterMark;
		m_nIoThrNum = threadnum;
		m_nDefRecvBuf = nDefRecvBuf;
		m_nMaxRecvBuf = nMaxRecvBuf;
		m_nMaxSendQue = nMaxSendQue;

		bool bRet = false;

		// �������߳�loop
		EventLoopThreadPtr pMainLoopThr(new EventLoopThread(NULL, "mTcpCliMain"));
		EventLoop* pMainLoop = pMainLoopThr->startLoop();
		if (pMainLoop)
		{
			// ����Io�̳߳�
			EventLoopThreadPoolPtr pIoThrPool(new EventLoopThreadPool(pMainLoop, "mTcpCliIo"));
			if (pIoThrPool)
			{
				int nLoopNum = m_nIoThrNum;
				nLoopNum <= 0 ? nLoopNum = 1 : 1;
				nLoopNum > 128 ? nLoopNum = 128 : 1;
				pIoThrPool->setThreadNum(nLoopNum);
				pMainLoop->queueInLoop(std::bind(&EventLoopThreadPool::startThrPool, pIoThrPool));

				// ����mainloop
				main_loop_ = pMainLoopThr;
				//����Io�̳߳�
				io_thread_pool_ = pIoThrPool;

				bRet = true;
				exit_ = false;
			}
		}
		return bRet;
	}

	// ����ʼ��
	void mTcpClient::UnInitTcpClient()
	{
		exit_ = true;
		EventLoopThreadPtr pMainLoopThr(boost::any_cast<EventLoopThreadPtr>(main_loop_));
		if (pMainLoopThr)
		{
			pMainLoopThr->getloop()->queueInLoop(std::bind(&mTcpClient::ClearConnMap, shared_from_this()));
		}
	}

	void mTcpClient::AddConnectionInLoop(const ConnBaseInfoPtr& pConnBaseInfo)
	{
		if (pConnBaseInfo)
		{
			EventLoopThreadPoolPtr pIoThrPool(boost::any_cast<EventLoopThreadPoolPtr>(io_thread_pool_));
			if (pIoThrPool)
			{
				pConnBaseInfo->conn_uuid = MakeSnowId(0, g_nSequnceId);
				pConnBaseInfo->conn_begin_tm_ms = Timestamp::GetCurrentTimeMs();
				pConnBaseInfo->conn_status = CONNSTATUS_CONNECTING;
				pConnBaseInfo->conn_status_desc = "connecting";
				// ����tcpclient
				// todo:֧��ipv6
				std::string strIp = "";
				if (exit_ || !domain_resolver(pConnBaseInfo->conn_ip, strIp))
				{
					// ����Ѿ�����ʼ����, Ϊ���ÿ��ܴ��ڵĻ�ѹ����������ȫ���ص�, ���������ݴ���
					// ��������ʧ��,ֱ�ӻص�����ʧ��
					pConnBaseInfo->conn_status = CONNSTATUS_CONNECTFAIL;
					pConnBaseInfo->conn_end_tm_ms = Timestamp::GetCurrentTimeMs();

					if (m_pfunc_on_connection)
					{
						pConnBaseInfo->conn_status_desc = exit_ ? "connect fail:Tcpclient has exited" : "connect fail:Domain resolve fail";
						std::string strIpPort = pConnBaseInfo->conn_ip + ":" + std::to_string(pConnBaseInfo->conn_port);
						m_pfunc_on_connection(m_pInvoker, pConnBaseInfo, false, strIpPort.c_str());
					}
				}
				else
				{
					InetAddress ipAddr(strIp, pConnBaseInfo->conn_port);
					xTcpClientPtr client(new xTcpClient(m_pInvoker,
						pConnBaseInfo, pIoThrPool->getNextLoop(), ipAddr,
						m_pfunc_on_connection, m_pfunc_on_readmsg_tcp,
						m_pfunc_on_sendok, m_pfunc_on_senderr, m_pfunc_on_highwatermark,
						m_nDefRecvBuf, m_nMaxRecvBuf, m_nMaxSendQue));
					if (client)
					{
						// �������Ӷ���
						pConnBaseInfo->conn_obj = client;
						AddConnToMap(pConnBaseInfo);
						// ����mTcpClient����,��ֹ����ʱ��������
						client->tie(shared_from_this());
						// ��������
						client->Connect();
					}
				}
			}
		}
	}
	// �������
	void mTcpClient::AddConnection(const ConnBaseInfoPtr& pConnBaseInfo)
	{
		EventLoopThreadPtr pMainLoopThr(boost::any_cast<EventLoopThreadPtr>(main_loop_));
		if (pMainLoopThr)
		{
			pMainLoopThr->getloop()->queueInLoop(std::bind(&mTcpClient::AddConnectionInLoop, shared_from_this(), pConnBaseInfo));
		}
	}

	void mTcpClient::DelConnectionInLoop(const ConnBaseInfoPtr& pConnBaseInfo)
	{
		if (pConnBaseInfo)
		{
			if (DelConnFromMap(pConnBaseInfo))
			{
				if (!pConnBaseInfo->conn_obj.empty())
				{
					xTcpClientPtr client(boost::any_cast<xTcpClientPtr>(pConnBaseInfo->conn_obj));
					if (client)
					{
						// disconnect conn
						client->DisConnect();
						// stop client
						client->Stop();
					}
					pConnBaseInfo->conn_obj = nullptr;
				}
			}
		}
	}

	// ɾ������
	void mTcpClient::DelConnection(const ConnBaseInfoPtr& pConnBaseInfo)
	{
		EventLoopThreadPtr pMainLoopThr(boost::any_cast<EventLoopThreadPtr>(main_loop_));
		if (pMainLoopThr)
		{
			pMainLoopThr->getloop()->queueInLoop(std::bind(&mTcpClient::DelConnectionInLoop, shared_from_this(), pConnBaseInfo));
		}
	}

	// ���ø�ˮλֵ
	void mTcpClient::SetHighWaterMark(const ConnBaseInfoPtr& pConnBaseInfo, size_t highwatermark)
	{
		if (pConnBaseInfo && !pConnBaseInfo->conn_obj.empty())
		{
			xTcpClientPtr client(boost::any_cast<xTcpClientPtr>(pConnBaseInfo->conn_obj));
			if (client)
			{
				client->SetHighWaterMark(highwatermark);
			}
		}
	}

	size_t mTcpClient::GetTotalConnCnt() const
	{
		return connections_.size();
	}

	//��ͣ��ǰTCP���ӵ����ݽ���
	void mTcpClient::SuspendTcpConnRecv(const ConnBaseInfoPtr& pConnBaseInfo, int delay)
	{
		if (pConnBaseInfo && !pConnBaseInfo->conn_obj.empty())
		{
			xTcpClientPtr client(boost::any_cast<xTcpClientPtr>(pConnBaseInfo->conn_obj));
			if (client)
			{
				client->SuspendTcpConnRecv(delay);
			}
		}
	}

	//�ָ���ǰTCP���ӵ����ݽ���
	void mTcpClient::ResumeTcpConnRecv(const ConnBaseInfoPtr& pConnBaseInfo)
	{
		if (pConnBaseInfo && !pConnBaseInfo->conn_obj.empty())
		{
			xTcpClientPtr client(boost::any_cast<xTcpClientPtr>(pConnBaseInfo->conn_obj));
			if (client)
			{
				client->ResumeTcpConnRecv();
			}
		}
	}

	// ������Ϣ 
	int  mTcpClient::SendMsgWithTcpClient(const ConnBaseInfoPtr& pConnBaseInfo, const boost::any& params, const char* szMsg, size_t nMsgLen, int timeout_ms, bool bKeepAlive)
	{
		if (pConnBaseInfo && !pConnBaseInfo->conn_obj.empty())
		{
			xTcpClientPtr client(boost::any_cast<xTcpClientPtr>(pConnBaseInfo->conn_obj));
			if (client)
			{
				return client->SendTcpRequest(params, szMsg, nMsgLen, timeout_ms, bKeepAlive);
			}
		}
		return 1;
	}

	void mTcpClient::AddConnToMap(const ConnBaseInfoPtr& pConnBaseInfo)
	{
		std::lock_guard<std::mutex> lock(lock_);
		connections_.insert(std::make_pair(pConnBaseInfo->conn_uuid, pConnBaseInfo));
	}

	bool mTcpClient::DelConnFromMap(const ConnBaseInfoPtr& pConnBaseInfo)
	{
		std::lock_guard<std::mutex> lock(lock_);
		return connections_.erase(pConnBaseInfo->conn_uuid) > 0;
	}

	void mTcpClient::ClearConnMap()
	{
		std::lock_guard<std::mutex> lock(lock_);
		
		for (auto &it : connections_)
		{
			if (!it.second->conn_obj.empty())
			{
				xTcpClientPtr client(boost::any_cast<xTcpClientPtr>(it.second->conn_obj));
				if (client)
				{
					// disconnect conn
					client->DisConnect();
					// stop client
					client->Stop();
				}
				it.second->conn_obj = nullptr;
			}
		}
		connections_.clear();
	}
}
}
