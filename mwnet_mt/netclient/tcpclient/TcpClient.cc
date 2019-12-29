#include "TcpClient.h"

#include <mwnet_mt/net/TcpClient.h>
#include <mwnet_mt/base/Thread.h>
#include <mwnet_mt/base/Atomic.h>
#include <mwnet_mt/net/EventLoopThread.h>
#include <mwnet_mt/net/EventLoop.h>
#include <mwnet_mt/net/InetAddress.h>
#include <sys/time.h>
#include <memory>  
#include <iostream>

using namespace mwnet_mt;
using namespace mwnet_mt::net;

namespace MWNET_MT
{
namespace CLIENT
{
	AtomicInt64 g_nSequnceId;
	//����ѩ��,1���ڲ��ظ�
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
	void xdef_onConnection(const TcpConnectionPtr& conn)
	{
		TcpConnectionPtr conn_bak = conn;
		if (nullptr == conn)
		{
			//printf("xxxxxx--nullptr == conn\n");
			return;
		}

		if (conn->connected())
		{
			//printf("xxxxxx--UP, %p\n", reinterpret_cast<void*>(&conn_bak));
		}
		else if (conn->disconnected())
		{
			//printf("xxxxxx--DWON, %p\n", reinterpret_cast<void*>(&conn_bak));
		}
		else
		{
			//printf("xxxxxx-----����������������%d\n", conn->get_state());
		}
		return;
	}

	typedef std::weak_ptr<TcpConnection> WeakTcpConnectionPtr;
	class xTcpClient
		: public std::enable_shared_from_this<xTcpClient>
	{
	public:
		xTcpClient(void* pInvoker, EventLoop* loop, const InetAddress& svrAddr, 
						pfunc_on_connection pOnConnection,
						pfunc_on_readmsg_tcpclient pOnReadMsg,
						pfunc_on_sendok pOnSendOk,
						pfunc_on_highwatermark pOnHighWaterMark,
						size_t nDefRecvBuf,	size_t nMaxRecvBuf, size_t nMaxSendQue)
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
			client_.setConnectionCallback(std::bind(&xTcpClient::onConnection, this, _1));
			//client_.setConnectionCallback(xdef_onConnection);

			client_.setMessageCallback(std::bind(&xTcpClient::onMessage, this, _1, _2, _3, std::placeholders::_4));
			client_.setWriteCompleteCallback(std::bind(&xTcpClient::onWriteOk, this, _1, _2));
		}

		~xTcpClient()
		{
		}
		
		void Connect(const XTcpClientPtr& self)
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

		// ���ø�ˮλֵ
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
			// ����ʧ�ܡ���������
			if (!conn) 
			{
				if (m_pfunc_on_connection)
				{
					m_pfunc_on_connection(m_pInvoker, 0, false, 
						client_.getInetAddress().toIpPort().c_str(),
						client_.getInetAddress().toIp().c_str(),
						client_.getInetAddress().toPort());
				}

				if (self_) self_.reset();
				return;
			}
			
			if (conn->connected())
			{
				weak_conn_ = conn;
				conn_status_ = true;
				
				uint64_t conn_uuid = MakeSnowId(0, g_nSequnceId);
				conn->setConnuuid(conn_uuid);
				
				conn->setHighWaterMarkCallback(std::bind(&xTcpClient::onHighWater, shared_from_this(), _1, _2), highWaterMark_);
							
				if (m_pfunc_on_connection)
				{
					int nRet = m_pfunc_on_connection(m_pInvoker, conn->getConnuuid(), true,
                                                     conn->peerAddress().toIpPort().c_str(),
                                                     conn->peerAddress().toIp().c_str(),
                                                     conn->peerAddress().toPort());
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
					m_pfunc_on_connection(m_pInvoker, conn->getConnuuid(), false,
                                          conn->peerAddress().toIpPort().c_str(),
                                          conn->peerAddress().toIp().c_str(),
                                          conn->peerAddress().toPort());
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
			
				int nRet = m_pfunc_on_readmsg_tcp(m_pInvoker, conn->getConnuuid(), RecvMsg.data(), RecvMsg.size(), nReadedLen);
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

			int nRet = 0;
			if (m_pfunc_on_highwatermark)
			{
				nRet = m_pfunc_on_highwatermark(m_pInvoker, conn->getConnuuid(), highWaterMark);
			}

			if (nRet < 0 || conn->bNeedClose())
			{
				conn->shutdown();
				//conn->forceCloseWithDelay(1.0);  // > round trip of the whole Internet.
				conn->forceClose();
			}
		}

	private:
		// �ϲ���õ���ָ��
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
		XTcpClientPtr self_;
	};


////////////////////////////////////////////////////////////////////////////////////////////////
	typedef std::shared_ptr<EventLoopThread> EventLoopThreadPtr;

	NetClient::NetClient()
	:
		client_(nullptr),
		main_loop_(nullptr)
	{
		// ����һ���߳̽�loop�����߳���
		EventLoopThreadPtr pLoopThr(new EventLoopThread(NULL, "main_loop"));
		EventLoop* pLoop = pLoopThr->startLoop();
		if (pLoop)
		{
			// ����mainloop
			main_loop_ = pLoopThr;
		}
	}
	NetClient::~NetClient()
	{	
	}

	// ��ʼ����  ���跴��ʼ����ֱ������
	bool NetClient::InitTcpClient(void* pInvoker, const std::string& ip, uint16_t port,
									pfunc_on_connection pOnConnection,
									pfunc_on_readmsg_tcpclient pOnReadMsg,
									pfunc_on_sendok pOnSendOk,
									pfunc_on_highwatermark pOnHighWaterMark,
									size_t nDefRecvBuf,	size_t nMaxRecvBuf, size_t nMaxSendQue)
	{
		m_pInvoker = pInvoker;
		m_pfunc_on_connection = pOnConnection;
		m_pfunc_on_readmsg_tcp = pOnReadMsg;
		m_pfunc_on_sendok = pOnSendOk;
		m_pfunc_on_highwatermark = pOnHighWaterMark;
		m_nDefRecvBuf = nDefRecvBuf;m_nMaxRecvBuf = nMaxRecvBuf;m_nMaxSendQue=nMaxSendQue;

		// �رվɵ�tcpclient
		DisConnect();
				
		bool bRet = false;	

		EventLoopThreadPtr pLoopThr(boost::any_cast<EventLoopThreadPtr>(main_loop_));
		if (pLoopThr)
		{
			// ����tcpclient
			InetAddress connect_ip_port(ip, port);
			std::lock_guard<std::mutex> tmp_lock(lock_);
			client_.reset(new xTcpClient(pInvoker, pLoopThr->getloop(), connect_ip_port,
										pOnConnection, pOnReadMsg, pOnSendOk, pOnHighWaterMark,
										nDefRecvBuf, nMaxRecvBuf, nMaxSendQue));
			if (client_)	
			{
				// ��������
				client_->Connect(client_);
					
				bRet = true;
			}
		}
		return bRet;
	}

	
	// ���ø�ˮλֵ
	void  NetClient::SetHighWaterMark(size_t highwatermark)
	{
		std::lock_guard<std::mutex> tmp_lock(lock_);
		if (client_) client_->SetHighWaterMark(highwatermark);
	}
	
	// ��������
	int   NetClient::SendTcpRequest(const boost::any& params, const char* szMsg, size_t nMsgLen, bool bKeepAlive)
	{		
		std::lock_guard<std::mutex> tmp_lock(lock_);
		return client_?client_->SendTcpRequest(params, szMsg, nMsgLen, bKeepAlive):-1;
	}

	int64_t NetClient::getConnuuid()
	{
		std::lock_guard<std::mutex> tmp_lock(lock_);
		if (client_) return client_->getConnuuid();
		return 0;
	}

	std::string NetClient::getIpPort()
	{
		std::lock_guard<std::mutex> tmp_lock(lock_);
		if (client_) return client_->getIpPort();
		return "";
	}

	// ��������
	void  NetClient::ReConnect(const std::string& ip, uint16_t port)
	{
		// �رվɵ�tcpclient
		DisConnect();

		EventLoopThreadPtr pLoopThr(boost::any_cast<EventLoopThreadPtr>(main_loop_));
		if (pLoopThr)
		{
			// ����tcpclient
			InetAddress connect_ip_port(ip, port);
			std::lock_guard<std::mutex> tmp_lock(lock_);
			client_.reset(new xTcpClient(m_pInvoker, pLoopThr->getloop(), connect_ip_port,
										m_pfunc_on_connection, m_pfunc_on_readmsg_tcp, 
										m_pfunc_on_sendok, m_pfunc_on_highwatermark,
										m_nDefRecvBuf, m_nMaxRecvBuf, m_nMaxSendQue));
			if (client_)	
			{
				// ��������
				client_->Connect(client_);
			}
		}
	}

	// ����
	void  NetClient::Connect()
	{
		std::lock_guard<std::mutex> tmp_lock(lock_);
		if (client_) client_->Connect(client_);
	}
		
	// �Ͽ�����
	void   NetClient::DisConnect()
	{
		std::lock_guard<std::mutex> tmp_lock(lock_);

		if (client_) 
		{
			// disconnect conn
			client_->DisConnect();
			// stop client
			client_->Stop();
			// reset it inloop
			//XTcpClientPtr tmp(client_);
			ResetClient(client_);
			client_.reset();
		}
	}
	void  NetClient::ResetClient(XTcpClientPtr& xclient)
	{
		EventLoopThreadPtr pLoopThr(boost::any_cast<EventLoopThreadPtr>(main_loop_));
		if (pLoopThr)
		{
			//pLoopThr->getloop()->runAfter(2.0, std::bind(&NetClient::ResetClientInLoop, this, xclient));
			pLoopThr->getloop()->queueInLoop(std::bind(&NetClient::ResetClientInLoop, this, xclient));
		}
	}
	void  NetClient::ResetClientInLoop(XTcpClientPtr& xclient)
	{
		if (xclient) 
		{
			xclient.reset();
		}
	}
}
}
