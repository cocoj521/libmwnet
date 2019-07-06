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
		j |= static_cast<int64_t>(nYear& 0x7f) << 57;	//year 0~99
		j |= static_cast<int64_t>(nMonth & 0x0f) << 53;//month 1~12
		j |= static_cast<int64_t>(nDay & 0x1f) << 48;//day 1~31
		j |= static_cast<int64_t>(nHour & 0x1f) << 43;//hour 0~24
		j |= static_cast<int64_t>(nMin & 0x3f) << 37;//min 0~59
		j |= static_cast<int64_t>(nSec & 0x3f) << 31;//second 0~59
		j |= static_cast<int64_t>(nNodeid & 0x01fff) << 18;//nodeid 1~8000
		j |= static_cast<int64_t>(nNo & 0x03ffff);	//seqid,0~0x03ffff
	
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
			//printf("xxxxxx-----。。。。。。。。%d\n", conn->get_state());
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

			client_.setMessageCallback(std::bind(&xTcpClient::onMessage, this, _1, _2, _3));
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

		// 设置高水位值
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
			// 连接失败。。。。。
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
				
				uint64_t conn_uuid = MakeConnuuid(0, g_nSequnceId);
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

		void onMessage(const TcpConnectionPtr& conn, Buffer* buf, Timestamp time)
		{
			if (!conn) return;
						
			if (m_pfunc_on_readmsg_tcp)
			{
				int nReadedLen = 0;
				StringPiece RecvMsg = buf->toStringPiece();
			
				int nRet = m_pfunc_on_readmsg_tcp(m_pInvoker, conn->getConnuuid(), RecvMsg.data(), RecvMsg.size(), nReadedLen);
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
					if (buf->readableBytes() >= maxRecvBuf_)
					{
						nRet = -1;
						// LOG_WARN<<"[TCP][RECV]["<<conn->getConnuuid()<<"]\nillegal data size:"<<buf->readableBytes()<<",maybe too long\n";
					}
				}
				// 判断返回值，并做不同的处理
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
		// 上层调用的类指针
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
		// 申请一个线程将loop跑在线程里
		EventLoopThreadPtr pLoopThr(new EventLoopThread(NULL, "main_loop"));
		EventLoop* pLoop = pLoopThr->startLoop();
		if (pLoop)
		{
			// 保存mainloop
			main_loop_ = pLoopThr;
		}
	}
	NetClient::~NetClient()
	{	
	}

	// 初始化。  如需反初始化，直接析构
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

		// 关闭旧的tcpclient
		DisConnect();
				
		bool bRet = false;	

		EventLoopThreadPtr pLoopThr(boost::any_cast<EventLoopThreadPtr>(main_loop_));
		if (pLoopThr)
		{
			// 创建tcpclient
			InetAddress connect_ip_port(ip, port);
			std::lock_guard<std::mutex> tmp_lock(lock_);
			client_.reset(new xTcpClient(pInvoker, pLoopThr->getloop(), connect_ip_port,
										pOnConnection, pOnReadMsg, pOnSendOk, pOnHighWaterMark,
										nDefRecvBuf, nMaxRecvBuf, nMaxSendQue));
			if (client_)	
			{
				// 建立连接
				client_->Connect(client_);
					
				bRet = true;
			}
		}
		return bRet;
	}

	
	// 设置高水位值
	void  NetClient::SetHighWaterMark(size_t highwatermark)
	{
		std::lock_guard<std::mutex> tmp_lock(lock_);
		if (client_) client_->SetHighWaterMark(highwatermark);
	}
	
	// 发送请求
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

	// 重新连接
	void  NetClient::ReConnect(const std::string& ip, uint16_t port)
	{
		// 关闭旧的tcpclient
		DisConnect();

		EventLoopThreadPtr pLoopThr(boost::any_cast<EventLoopThreadPtr>(main_loop_));
		if (pLoopThr)
		{
			// 创建tcpclient
			InetAddress connect_ip_port(ip, port);
			std::lock_guard<std::mutex> tmp_lock(lock_);
			client_.reset(new xTcpClient(m_pInvoker, pLoopThr->getloop(), connect_ip_port,
										m_pfunc_on_connection, m_pfunc_on_readmsg_tcp, 
										m_pfunc_on_sendok, m_pfunc_on_highwatermark,
										m_nDefRecvBuf, m_nMaxRecvBuf, m_nMaxSendQue));
			if (client_)	
			{
				// 建立连接
				client_->Connect(client_);
			}
		}
	}

	// 连接
	void  NetClient::Connect()
	{
		std::lock_guard<std::mutex> tmp_lock(lock_);
		if (client_) client_->Connect(client_);
	}
		
	// 断开连接
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
