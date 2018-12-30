#include "TcpClient.h"

//////////////////////////////////////////////////////////////////////////
#include <mwnet_mt/net/TcpClient.h>
#include <mwnet_mt/base/Atomic.h>
#include <mwnet_mt/base/Thread.h>
#include <mwnet_mt/base/ThreadLocalSingleton.h>
#include <mwnet_mt/net/EventLoop.h>
#include <mwnet_mt/net/EventLoopThreadPool.h>
#include <mwnet_mt/net/EventLoopThread.h>
#include <mwnet_mt/net/InetAddress.h>

#include <boost/ptr_container/ptr_vector.hpp>
#include <boost/circular_buffer.hpp>
#include <boost/unordered_set.hpp>
#include <boost/version.hpp>
#include <boost/bind.hpp>

#include <utility>
#include <iostream>
#include <stdio.h>
#include <unistd.h>

#include <vector>
#include <map>

#include <cstddef>
#include <memory>
#include <thread>
#include <chrono>

#include <boost/any.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>

//////////////////////////////////////////////////////////////////////////

#include <mutex>
#include <atomic> 

#include <string>
#include <functional>



using namespace mwnet_mt;
using namespace mwnet_mt::net;


namespace MWNET_MT
{
	namespace CLIENT
	{

		//////////////////////////////////////////////////////////////////////////
		//////////////////////////////////////////////////////////////////////////

	// 	std::shared_ptr<NetTcpClientInterface> get_md_cli(uint16_t clients = 5, size_t wnd_size = 100);
	// 	int md_cli_init(int thread_num);
	// 	int md_cli_uninit();


		//////////////////////////////////////////////////////////////////////////

		class md_cli_wnd
		{
		public:
			md_cli_wnd()
			{
			}
			~md_cli_wnd()
			{
				clear();
			}
			int64_t total()
			{
				if (m_total.get() < 0)
					return 0;
				return m_total.get();
			}

			void del()
			{
				m_remain.decrement();
				m_total.decrement();
			}
			void add()
			{
				m_remain.increment();
				m_total.increment();
			}
			void clear()
			{
				m_total.getAndSet(m_total.get() - m_remain.get());
				m_remain.getAndSet(0);
			}

		private:
			AtomicInt64 m_total;		// �ܰ�����
			AtomicInt64 m_remain;		// δȷ�ϰ�
		};

		typedef std::weak_ptr<TcpConnection> STDWeakTcpConnectionPtr;

		class NetTcpClient
			: public NetTcpClientInterface
			, public std::enable_shared_from_this<NetTcpClient>
		{
		public:

			const static int kHeaderLen = sizeof(int32_t);
			const static int kMaxMessageLen = 10 * 1024 * 1024; // same as codec_stream.h kDefaultTotalBytesLimit

			enum ErrorCode
			{
				kNoError = 0,
				kInvalidLength = -1,
				kCheckSumError = -2,
				kInvalidNameLen = -3,
				kUnknownMessageType = -4,
				kParseError = -5,
			};

			/*
			clients �ͻ��˵ĸ���
			wnd_size ���ڴ�С,�����ô�������û���أ����޷��ٷ�������
			*/
			NetTcpClient(uint16_t clients = 4,
				size_t wnd_size = 100,
				size_t nDefRecvBuf = 4 * 1024,		/*Ĭ�Ͻ���buf*/
				size_t nMaxRecvBuf = 4 * 1024,		/*������buf*/
				size_t nMaxSendQue = 512			/*����Ͷ���*/);
			~NetTcpClient();

			int set_callback(const void* pInvokePtr, std::shared_ptr<NetTcpClientCallbackInterface> callback);
			/*
			Ĭ�ϲ��Զ�����
			*/
			int set_auto_reconnect(bool flag = true);
			/*
			���ӽ����ɹ�֮����ܵ��ã�
			Ĭ��tcp_delay
			*/
			int set_tcp_delay(bool flag = true);

			int conn(std::string ip, uint16_t port);
			int stop();

			int send(const boost::any& conn, std::string data);
			int send(const boost::any& conn, const char* data, size_t len);
			int send(int64_t uuid, const boost::any& param, const char* data, size_t len);

			int send(std::string data);
			int send(const char* data, size_t len);

			static EventLoopThreadPool& get_loop_instance();
			static EventLoop& get_loop();

			const void* m_pInvokePtr;

			int get_node_num() { return m_nNodeNum; }
			void set_node_num(int nNodeNum) { m_nNodeNum = nNodeNum; }

			void set_buf_info(size_t nDefRecvBuf,	/*Ĭ�Ͻ���buf*/
				size_t nMaxRecvBuf,					/*������buf*/
				size_t nMaxSendQue					/*����Ͷ���*/)
			{
				m_nDefRecvBuf = nDefRecvBuf;
				m_nMaxRecvBuf = nMaxRecvBuf;
				m_nMaxSendQue = nMaxSendQue;
			}

			// ��ȡ��ǰ����״̬
			bool state();
			// ��ȡ��ǰ���ջ����С
			size_t recvbuf_size();
			// ��ȡ��ǰ���ڴ�С
			size_t cur_wnd_size();

			void	setConnuuid(int64_t conn_uuid) { m_conn_uuid = conn_uuid; }
			int64_t getConnuuid() const { return m_conn_uuid; }

		private:
			int conn2(std::string ip, uint16_t port);
			void add_cli(std::shared_ptr<TcpClient>& cli_spr);
			std::mutex m_lock_cli;

			int64_t m_conn_uuid;

			std::shared_ptr<NetTcpClientCallbackInterface> m_callback;
			// std::shared_ptr<TcpClient> m_client_spr;
			// TcpConnectionPtr m_cnn;
			std::vector<std::shared_ptr<TcpClient>> m_vec_cli_spr;
			size_t m_wnd_size;
			size_t m_clients;

			size_t m_nDefRecvBuf;
			size_t m_nMaxRecvBuf;
			size_t m_nMaxSendQue;

			std::atomic<size_t> m_nCurRecvBuf;

			int m_nNodeNum;

			std::mutex m_lock_cnn_wnd;
			std::map<TcpConnectionPtr, std::shared_ptr<md_cli_wnd>> m_map_cnn_wnd;

			//	std::map<std::shared_ptr<TcpClient>, TcpConnectionPtr> m_map_cli_cnn;

			TcpConnectionPtr get_next_cnn();
			void add_cnn_wnd(const TcpConnectionPtr& cnn);
			void del_cnn_wnd(const TcpConnectionPtr& cnn);
			bool update_cnn_wnd(const TcpConnectionPtr& cnn);
			bool clear_cnn_wnd(const TcpConnectionPtr& cnn);

		private:
			void on_cnn(const TcpConnectionPtr& conn);
			void on_recv(const TcpConnectionPtr& conn, Buffer* buf, Timestamp dt);
			void on_send(const TcpConnectionPtr& conn);
		};
	}
}

//////////////////////////////////////////////////////////////////////////

// using namespace std::placeholders;

namespace MWNET_MT
{
	namespace CLIENT
	{
		// AtomicInt64 md_cli_wnd::m_total;


		// tcpclientʹ��
		//////////////////////////////////////////////////////////////////////////
		//////////////////////////////////////////////////////////////////////////
		class NetTcpClientMgrCallback;


		NetTcpClientMgr::NetTcpClientMgr()
			: m_port(static_cast<uint16_t>(11100))
			, m_ip("127.0.0.1")
			, m_pInvokePtr(NULL)
			, m_clients(static_cast<int>(4))
			, m_wnd_size(static_cast<int>(30))
			, m_nIdelTime(0)
			, m_nDefRecvBuf(4 * 1024)
			, m_nMaxRecvBuf(4 * 1024)
			, m_nMaxSendQue(512)
		{}
		NetTcpClientMgr::~NetTcpClientMgr()
		{}
// 		NetTcpClientMgr& NetTcpClientMgr::instance()
// 		{
// 			static NetTcpClientMgr cli;
// 			return cli;
// 			// return boost::container::container_detail::singleton_default<RmsCli>::instance();
// 		}

		uint64_t MakeConnuuid(uint16_t unique_node_id)
		{
			unsigned int nYear = 0;
			unsigned int nMonth = 0;
			unsigned int nDay = 0;
			unsigned int nHour = 0;
			unsigned int nMin = 0;
			unsigned int nSec = 0;
			unsigned int nNodeid = unique_node_id;
			static AtomicInt64 g_nSequnceId;
			unsigned int nNo = static_cast<unsigned int>(g_nSequnceId.getAndAdd(1) % (0x03ffff));

			struct timeval tv;
			struct tm	   tm_time;
			gettimeofday(&tv, NULL);
			localtime_r(&tv.tv_sec, &tm_time);

			/*
			time_t t = time(NULL);
			struct tm tm_time = *localtime(&t);
			*/
			nYear = tm_time.tm_year + 1900;
			nYear = nYear % 100;
			nMonth = tm_time.tm_mon + 1;
			nDay = tm_time.tm_mday;
			nHour = tm_time.tm_hour;
			nMin = tm_time.tm_min;
			nSec = tm_time.tm_sec;

			int64_t j = 0;
			j |= static_cast<int64_t>(nYear & 0x7f) << 57;   //year 0~99
			j |= static_cast<int64_t>(nMonth & 0x0f) << 53;//month 1~12
			j |= static_cast<int64_t>(nDay & 0x1f) << 48;//day 1~31
			j |= static_cast<int64_t>(nHour & 0x1f) << 43;//hour 0~24
			j |= static_cast<int64_t>(nMin & 0x3f) << 37;//min 0~59
			j |= static_cast<int64_t>(nSec & 0x3f) << 31;//second 0~59
			j |= static_cast<int64_t>(nNodeid & 0x01fff) << 18;//nodeid 1~8000
			j |= static_cast<int64_t>(nNo & 0x03ffff);	//seqid,0~0x03ffff

			return j;
		}


		NetTcpClientMgrContaner::NetTcpClientMgrContaner()
			: m_tcpclient_readmsg_cb(NULL)
			, m_tcpclient_connect_cb(NULL)
			, m_tcpclient_send_cb(NULL)
		{}
		NetTcpClientMgrContaner::~NetTcpClientMgrContaner()
		{}

		void NetTcpClientMgrCallback::on_cnn(const void* pInvokePtr, int64_t uuid, int nNodeNum,
			const boost::any& conn, const boost::any& param, int ret, std::string info)
		{
			if (NetTcpClientMgrContaner::instance().m_tcpclient_connect_cb)
				NetTcpClientMgrContaner::instance().m_tcpclient_connect_cb(const_cast<void*>(pInvokePtr), uuid, param, ret, info.c_str());
		}

		void NetTcpClientMgrCallback::on_send(const void* pInvokePtr, int64_t uuid, int nNodeNum,
			const boost::any& conn, const boost::any& param,
			std::shared_ptr<NetTcpClientInterface> c, size_t remain_len)
		{
			if (NetTcpClientMgrContaner::instance().m_tcpclient_send_cb)
				NetTcpClientMgrContaner::instance().m_tcpclient_send_cb(const_cast<void*>(pInvokePtr), uuid, param);
		}

		int NetTcpClientMgrCallback::on_recv(const void* pInvokePtr, int64_t uuid, int nNodeNum,
			const boost::any& conn, const boost::any& param,
			std::shared_ptr<NetTcpClientInterface> c,
			const char* data, size_t len, int& nReadedLen, int64_t cur_time)
		{
			// nReadedLen = len;

			if (NetTcpClientMgrContaner::instance().m_tcpclient_readmsg_cb)
				NetTcpClientMgrContaner::instance().m_tcpclient_readmsg_cb(const_cast<void*>(pInvokePtr), uuid, param, data, static_cast<int>(len), nReadedLen);

			return 0;
		}

		// NetTcpClientMgr ����
		uint64_t NetTcpClientMgrContaner::add(std::shared_ptr<NetTcpClientMgr> item)
		{
			std::lock_guard<std::mutex> lock(m_lock);

			auto uuid = MakeConnuuid();
			auto ite = m_map_mgr.find(uuid);
			int cnt = 10;
			while (ite != m_map_mgr.end())
			{
				uuid = MakeConnuuid();
				if (cnt-- < 0)
					return 0;
			}
			m_map_mgr[uuid] = item;
			return uuid;

		}
		void NetTcpClientMgrContaner::del(std::shared_ptr<NetTcpClientMgr> item)
		{
			std::lock_guard<std::mutex> lock(m_lock);
			for (auto ite = m_map_mgr.begin();
				ite != m_map_mgr.end(); ++ite)
			{
				if (ite->second == item)
				{
					m_map_mgr.erase(ite);
					return;
				}
			}
		}
		void NetTcpClientMgrContaner::del(uint64_t uuid)
		{
			std::lock_guard<std::mutex> lock(m_lock);
			if (uuid)
			{
				auto ite = m_map_mgr.find(uuid);
				if (ite != m_map_mgr.end())
				{
					auto mgr = ite->second;
					if (mgr)
					{
						auto cli = mgr->m_cli_spr;
						cli->stop();
					}
					m_map_mgr.erase(ite);
				}
			}
			else
			{// ȫ�����
				for (auto item : m_map_mgr)
				{
					auto mgr = item.second;
					if (mgr)
					{
						auto cli = mgr->m_cli_spr;
						cli->stop();
					}
				}
				m_map_mgr.clear();
			}
			
		}
		std::shared_ptr<NetTcpClientMgr> NetTcpClientMgrContaner::get(uint64_t uuid)
		{
			std::lock_guard<std::mutex> lock(m_lock);
			auto ite = m_map_mgr.find(uuid);
			if (ite != m_map_mgr.end())
				return ite->second;
			return nullptr;
		}
		std::shared_ptr<NetTcpClientInterface> NetTcpClientMgrContaner::get_cli(uint64_t cli_uuid, uint64_t cnn_uuid)
		{
			if (cli_uuid)
			{
				// �ƶ���cli_uuid
				std::lock_guard<std::mutex> lock(m_lock);
				auto ite = m_map_mgr.find(cli_uuid);
				if (ite == m_map_mgr.end())
				{
					return nullptr;
				}
				
				auto mgr = ite->second;
				if (mgr)
				{
					return mgr->m_cli_spr;
				}
				return nullptr;
			}

			//////////////////////////////////////////////////////////////////////////
			if (cnn_uuid)
			{
				// ָ��������
				std::map<uint64_t, std::shared_ptr<NetTcpClientMgr>> map_mgr;
				{
					std::lock_guard<std::mutex> lock(m_lock);
					map_mgr = m_map_mgr;
				}
				for (auto item : map_mgr)
				{
					auto cli = item.second->m_cli_spr;
					if (cli)
					{
						if (cnn_uuid == static_cast<uint64_t>(cli->getConnuuid()))
							return cli;
					}
				}
			}
			else
			{
				// δָ�����ӣ� �ǻ���ô������ֱ�ӷ���ʧ��
			}
			return nullptr;
		}
		bool NetTcpClientMgrContaner::empty()
		{
			std::lock_guard<std::mutex> lock(m_lock);
			return m_map_mgr.empty();
		}


		// rmsʹ��
		//////////////////////////////////////////////////////////////////////////
		//////////////////////////////////////////////////////////////////////////
		class RmsTcpClientCallback;


		RmsCli::RmsCli()
			: m_pInvokePtr(NULL)
			, m_clients(static_cast<int>(4))
			, m_wnd_size(static_cast<int>(30))
			, m_nIdelTime(0)
			, m_nDefRecvBuf(4 * 1024)
			, m_nMaxRecvBuf(4 * 1024)
			, m_nMaxSendQue(512)
		{}
		RmsCli::~RmsCli()
		{}
		RmsCli& RmsCli::instance()
		{
			static RmsCli cli;
			return cli;
			// return boost::container::container_detail::singleton_default<RmsCli>::instance();
		}


		void RmsTcpClientCallback::on_cnn(const void* pInvokePtr, int64_t uuid, int nNodeNum,
			const boost::any& conn, const boost::any& param, int ret, std::string info)
		{
			int nReadedLen = 0;
			if (RmsCli::instance().m_rms_cb)
				RmsCli::instance().m_rms_cb(pInvokePtr, nNodeNum, "on_cnn", ret, "", 0, nReadedLen, time(NULL));
		}

		void RmsTcpClientCallback::on_send(const void* pInvokePtr, int64_t uuid, int nNodeNum,
			const boost::any& conn, const boost::any& param, std::shared_ptr<NetTcpClientInterface> c, size_t remain_len)
		{
			int nReadedLen = 0;
			std::weak_ptr<NetTcpClientInterface> weakcli(c);
			if (RmsCli::instance().m_rms_cb)
				RmsCli::instance().m_rms_cb(pInvokePtr, nNodeNum, "on_send", 0, "", 0, nReadedLen, time(NULL));
		}

		int RmsTcpClientCallback::on_recv(const void* pInvokePtr, int64_t uuid, int nNodeNum,
			const boost::any& conn, const boost::any& param, std::shared_ptr<NetTcpClientInterface> c,
			const char* data, size_t len, int& nReadedLen, int64_t cur_time)
		{
			// nReadedLen = len;

			std::weak_ptr<NetTcpClientInterface> weakcli(c);
			if (RmsCli::instance().m_rms_cb)
				RmsCli::instance().m_rms_cb(pInvokePtr, nNodeNum, "on_recv", 0, data, len, nReadedLen, cur_time);

			return 0;
		}


		std::string RmsGetErrInfo(int err)
		{
			std::string str;
			if (0 == err)
				return "ok";
			if (-100 == err)
				return "wnd full";
			if (-1000 == err)
				return "cant get rmscli instance";
			if (-1001 == err)
				return "rmscli not init yet";

			return "failed";
		}

		/*
		���ܣ� ��ʼ��
		pInvokePtr �û��Զ�������
		thread_num �߳���(���ڲ����޸�)
		clients �ͻ�����Ŀ(���ڲ����޸�)
		wnd_size ������С(���ڲ����޸�)
		*/
		int RmsInitialise(const void* pInvokePtr, RmsCb cb, RMS_PARAMS& rms_param,
			int thread_num,
			size_t wnd_size,
			size_t nDefRecvBuf,		/*Ĭ�Ͻ���buf*/
			size_t nMaxRecvBuf,		/*������buf*/
			size_t nMaxSendQue		/*����Ͷ���*/)
		{
			md_cli_init(thread_num);

			int iClientsPerNode = rms_param.iClientsPerNode;
			if (iClientsPerNode <= 0)
				iClientsPerNode = 1;
			std::vector<RMS_LINKNODE>& vecLinkNode = rms_param.vecLinkNode;
			if (vecLinkNode.empty())
				return -1;

			for (auto node : vecLinkNode)
			{
				auto cli_spr = get_md_cli(static_cast<uint16_t>(iClientsPerNode), wnd_size);
				if (cli_spr)
				{
					RmsCli::instance().m_cb_spr = std::make_shared<RmsTcpClientCallback>();
					RmsCli::instance().m_pInvokePtr = pInvokePtr;
					RmsCli::instance().m_rms_cb = cb;

					RmsCli::instance().m_nDefRecvBuf = nDefRecvBuf;
					RmsCli::instance().m_nMaxRecvBuf = nMaxRecvBuf;
					RmsCli::instance().m_nMaxSendQue = nMaxSendQue;

					RmsCli::instance().m_map_index_cli_spr[node.nNodeNum] = cli_spr;
					RmsCli::instance().m_map_index_cli_spr[node.nNodeNum]->set_callback(pInvokePtr, RmsCli::instance().m_cb_spr);
					RmsCli::instance().m_map_index_cli_spr[node.nNodeNum]->set_buf_info(nDefRecvBuf, nMaxRecvBuf, nMaxSendQue);

					// Ĭ���Զ�����
					RmsCli::instance().m_map_index_cli_spr[node.nNodeNum]->set_auto_reconnect(true);
					RmsCli::instance().m_map_index_cli_spr[node.nNodeNum]->set_node_num(node.nNodeNum);

					RmsCli::instance().m_map_index_cli_spr[node.nNodeNum]->conn(node.szIp, static_cast<uint16_t>(node.nPort));
				}
			}

			return 0;
		}


		/*
		���ܣ� ֹͣ
		*/
		int RmsStop()
		{
			for (auto item : RmsCli::instance().m_map_index_cli_spr)
			{
				if (item.second)
				{
					item.second->stop();
				}
			}
			return -1;
		}


		/*
		���ܣ� ��ֹ��
		pInvokePtr �û��Զ�������
		thread_num �߳���(���ڲ����޸�)
		*/
		int RmsUnInitialise()
		{
			return md_cli_uninit();
		}

		/*
		���ܣ� ������Ϣ
		pInvokePtr �û��Զ�������
		thread_num �߳���(���ڲ����޸�)
		*/
		int RmsSend(int nNodeNum, const char *szData, const int nLen)
		{
			auto item = RmsCli::instance().m_map_index_cli_spr.find(nNodeNum);
			if (item != RmsCli::instance().m_map_index_cli_spr.end()
				&& item->second)
			{
				return item->second->send(szData, nLen);
			}
			else
				return -1001;

			return 0;
		}





		//////////////////////////////////////////////////////////////////////////
		//////////////////////////////////////////////////////////////////////////
		std::shared_ptr<NetTcpClientInterface> get_md_cli(uint16_t clients, size_t wnd_size)
		{
			return std::dynamic_pointer_cast<NetTcpClientInterface>(std::make_shared<NetTcpClient>(clients, wnd_size));
		}

		int md_cli_init(int thread_num)
		{
			NetTcpClient::get_loop_instance().setThreadNum(thread_num);
			NetTcpClient::get_loop().runInLoop(
				boost::bind(&EventLoopThreadPool::start, &NetTcpClient::get_loop_instance(),
					EventLoopThreadPool::ThreadInitCallback()));
			while (NetTcpClient::get_loop().queueSize() > 0)
				std::this_thread::sleep_for(std::chrono::milliseconds(10));
			return 0;
		}

		int md_cli_uninit()
		{
			NetTcpClient::get_loop().quit();
			return 0;
		}
		//////////////////////////////////////////////////////////////////////////

		// #include <boost/serialization/singleton.hpp>  
		EventLoopThreadPool& NetTcpClient::get_loop_instance()
		{
			// 	boost::serialization::singleton<EventLoop> loop;
			// 	boost::serialization::singleton<EventLoopThreadPool> threadPool(&loop, "g_cli_loop");
			static EventLoopThreadPool threadPool(&NetTcpClient::get_loop(), "g_cli_loop");

			return threadPool;
		}

		EventLoop& NetTcpClient::get_loop()
		{
			static EventLoopThread loopThread;
			static EventLoop* evl = loopThread.startLoop();
			return *evl;
		}

		int NetTcpClient::set_auto_reconnect(bool flag)
		{
			for (auto item : m_vec_cli_spr)
				item->enableRetry();
			return 0;
		}
		int NetTcpClient::set_tcp_delay(bool flag)
		{
			// 	if (m_cnn)
			// 		m_cnn->setTcpNoDelay(!flag);
			// 	else
			// 		return -1;

			return 0;
		}

		void NetTcpClient::on_send(const TcpConnectionPtr& conn_)
		{
			size_t remain_len = 0;
			if (m_callback)
			{
				STDWeakTcpConnectionPtr weakconn(conn_);
				m_callback->on_send(m_pInvokePtr, conn_->getConnuuid(), m_nNodeNum,
					weakconn, conn_->getContext(), std::dynamic_pointer_cast<NetTcpClientInterface>(shared_from_this()),
					remain_len);
			}

			clear_cnn_wnd(conn_);

		}
		void NetTcpClient::on_cnn(const TcpConnectionPtr& conn_)
		{
			if (conn_->connected())
			{
				if (m_conn_uuid <= 0)
				{
					uint64_t conn_uuid = MakeConnuuid();
					conn_->setConnuuid(conn_uuid);
					m_conn_uuid = conn_uuid;
				}
				else
				{
					conn_->setConnuuid(m_conn_uuid);
				}
				conn_->setDefRecvBuf(m_nDefRecvBuf);
				conn_->setMaxRecvBuf(m_nMaxRecvBuf);
				conn_->setMaxSendQue(m_nMaxSendQue);

				add_cnn_wnd(conn_);
				// m_cnn = conn;
				// conn->setTcpNoDelay(true);
				if (m_callback)
				{
					STDWeakTcpConnectionPtr weakconn(conn_);
					m_callback->on_cnn(m_pInvokePtr, conn_->getConnuuid(), m_nNodeNum,
						weakconn, conn_->getContext(), true, conn_->peerAddress().toIpPort().c_str());
				}
			}
			else
			{
				del_cnn_wnd(conn_);
				// m_cnn.reset();
				if (m_callback)
				{
					STDWeakTcpConnectionPtr weakconn(conn_);
					m_callback->on_cnn(m_pInvokePtr, conn_->getConnuuid(), m_nNodeNum, weakconn,
						conn_->getContext(), false, conn_->peerAddress().toIpPort().c_str());
				}
			}
		}

		void NetTcpClient::on_recv(const TcpConnectionPtr& conn_, Buffer* buf, Timestamp dt)
		{
			if (!conn_) return;;

			if (m_callback)
			{
				int nReadedLen = 0;
				StringPiece RecvMsg = buf->toStringPiece();

				STDWeakTcpConnectionPtr weakconn(conn_);

				// ���µ�ǰ��
				m_nCurRecvBuf = buf->readableBytes();

				int nRet = m_callback->on_recv(m_pInvokePtr, conn_->getConnuuid(), m_nNodeNum, weakconn,
					conn_->getContext(), std::dynamic_pointer_cast<NetTcpClientInterface>(shared_from_this()),
					RecvMsg.data(), RecvMsg.size(), nReadedLen, dt.microSecondsSinceEpoch());
				//�պö���������
				if (nReadedLen == RecvMsg.size())
				{
					buf->retrieveAll();
				}
				//ճ��/�а�
				else
				{
					//��buf�е�ָ��������ǰ�Ѵ�����ĵط�
					buf->retrieve(nReadedLen);

					// ��û�г���������BUF,�����κδ���...�Ƚ��������ٴ���,�����,��ֱ��close
					if (buf->readableBytes() >= m_nMaxRecvBuf)
					{
						nRet = -1;
					}
				}
				//�жϷ���ֵ��������ͬ�Ĵ���
				if (nRet < 0)
				{
					conn_->shutdown();
					//conn->forceCloseWithDelay(1.0);  // > round trip of the whole Internet.
					conn_->forceClose();
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


		NetTcpClient::NetTcpClient(uint16_t clients,
			size_t wnd_size,
			size_t nDefRecvBuf,		/*Ĭ�Ͻ���buf*/
			size_t nMaxRecvBuf,		/*������buf*/
			size_t nMaxSendQue			/*����Ͷ���*/)
		{
			m_pInvokePtr = NULL;

			m_wnd_size = wnd_size;
			m_clients = clients;
			m_nDefRecvBuf = nDefRecvBuf;
			m_nMaxRecvBuf = nMaxRecvBuf;
			m_nMaxSendQue = nMaxSendQue;

			m_nCurRecvBuf = 0;

			if (m_wnd_size <= 0)
				m_wnd_size = 1;
			if (m_clients <= 0)
				m_clients = 1;

			m_nNodeNum = -1;

			m_conn_uuid = 0;
		}
		NetTcpClient::~NetTcpClient()
		{
			m_callback.reset();
			// m_cnn.reset();

			for (auto item : m_vec_cli_spr)
			{
				item->stop();
			}
		}

		int NetTcpClient::set_callback(const void* pInvokePtr, std::shared_ptr<NetTcpClientCallbackInterface> callback)
		{
			m_callback = callback;
			m_pInvokePtr = pInvokePtr;
			return 0;
		}

		void NetTcpClient::add_cli(std::shared_ptr<TcpClient>& cli_spr)
		{
			std::lock_guard<std::mutex> lock(m_lock_cli);
			m_vec_cli_spr.push_back(cli_spr);
		}

		int NetTcpClient::conn2(std::string ip, uint16_t port)
		{
			for (size_t i = 0; i < m_clients; ++i)
			{
				EventLoop* loop = NetTcpClient::get_loop_instance().getNextLoop();
				std::shared_ptr<NetTcpClient> thiz = shared_from_this();
				loop->runInLoop(std::bind([thiz, loop, ip, port, &i]()
				{
					InetAddress net_addr(ip, port);
					std::shared_ptr<TcpClient> cli_spr(new TcpClient(loop,
						net_addr, std::string("cli_" + std::to_string(i)).c_str()));
					if (cli_spr)
					{
						thiz->add_cli(cli_spr);
						cli_spr->setConnectionCallback(std::bind(&NetTcpClient::on_cnn, thiz, std::placeholders::_1));
						cli_spr->setMessageCallback(std::bind(&NetTcpClient::on_recv, thiz, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
						cli_spr->setWriteCompleteCallback(std::bind(&NetTcpClient::on_send, thiz, std::placeholders::_1));
						cli_spr->connect();
						cli_spr->enableRetry();
					}

				}));

			}
			if (PRINT_LOG) std::cout << "conn size: " << m_vec_cli_spr.size() << std::endl;

			if (PRINT_LOG) std::cout << "conn " << ip << " " << port << std::endl;

			if (m_vec_cli_spr.size() <= 0)
				return -1;

			return 0;
		}

		int NetTcpClient::conn(std::string ip, uint16_t port)
		{
			if (!m_vec_cli_spr.empty())
				return 0;

			get_loop().runInLoop(std::bind(&NetTcpClient::conn2, shared_from_this(), ip, port));

			return 0;
		}
		int NetTcpClient::stop()
		{
			if (PRINT_LOG) std::cout << "md_cli::stop!!!!!!!!" << std::endl;
			for (auto item : m_vec_cli_spr)
			{
				item->disconnect();
				item.reset();
			}
			return 0;
		}

		int NetTcpClient::send(const boost::any& conn_, std::string data)
		{
			STDWeakTcpConnectionPtr weakcon = boost::any_cast<STDWeakTcpConnectionPtr>(conn_);
			TcpConnectionPtr c(weakcon.lock());
			if (c && c->connected())
			{
				if (false == update_cnn_wnd(c))
					return -100;

				c->send(data.c_str(), static_cast<int>(data.size()), nullptr);
			}
			else
				return -1;
			return 0;

		}
		int NetTcpClient::send(const boost::any& conn_, const char* data, size_t len)
		{
			STDWeakTcpConnectionPtr weakcon = boost::any_cast<STDWeakTcpConnectionPtr>(conn_);
			TcpConnectionPtr c(weakcon.lock());
			if (c && c->connected())
			{
				if (false == update_cnn_wnd(c))
					return -100;

				c->send(data, static_cast<int>(len), nullptr);

			}
			else
				return -1;
			return 0;

		}

		int NetTcpClient::send(int64_t uuid, const boost::any& param, const char* data, size_t len)
		{
			if (0 == uuid)
			{
				TcpConnectionPtr c = get_next_cnn();
				if (c && c->connected())
				{
					if (false == update_cnn_wnd(c))
						return -100;

					c->send(data, static_cast<int>(len), nullptr);
					c->setContext(param);
				}
				else
					return -1;
				return 0;

			}

			for (auto item : m_vec_cli_spr)
			{
				if (item)
				{
					auto c = item->connection();
					if (c && c->getConnuuid() == uuid)
					{
						if (c->connected())
						{
							if (false == update_cnn_wnd(c))
								return -100;
							// ���д���û��ȫ�����·��͵����ݲ������������Է��ͳɹ�
							// c->send(data, static_cast<int>(len));
							c->send(data, static_cast<int>(len), nullptr);
							c->setContext(param);
						}
						else
							return -1;
						return 0;
					}
				}

			}

			// if(PRINT_LOG) std::cout << "cant get connect by uuid:" << uuid << std::endl;
			return -2;

		}

		TcpConnectionPtr NetTcpClient::get_next_cnn()
		{
			if (m_vec_cli_spr.empty())
				return TcpConnectionPtr();

			static size_t index = 0;
			size_t cnt = (index++) % m_vec_cli_spr.size();

			return m_vec_cli_spr[cnt]->connection();
		}


		bool NetTcpClient::update_cnn_wnd(const TcpConnectionPtr& cnn)
		{
			std::lock_guard<std::mutex> lock(m_lock_cnn_wnd);
			auto ite = m_map_cnn_wnd.find(cnn);
			if (ite != m_map_cnn_wnd.end())
			{
				auto cnn_wnd_spr = ite->second;
				if (cnn_wnd_spr && cnn_wnd_spr->total() < static_cast<int64_t>(m_wnd_size))
				{
					cnn_wnd_spr->add();
					return true;
				}
			}
			return false;
		}
		bool NetTcpClient::clear_cnn_wnd(const TcpConnectionPtr& cnn)
		{
			std::lock_guard<std::mutex> lock(m_lock_cnn_wnd);
			if (PRINT_LOG) std::cout << "clear_cnn_wnd ========" << std::endl;
			auto ite = m_map_cnn_wnd.find(cnn);
			if (ite != m_map_cnn_wnd.end())
			{
				if (PRINT_LOG) std::cout << "clear_cnn_wnd cleared========" << std::endl;
				auto& cnn_wnd_spr = ite->second;
				cnn_wnd_spr->clear();
			}
			return false;
		}

		void NetTcpClient::add_cnn_wnd(const TcpConnectionPtr& cnn)
		{
			std::lock_guard<std::mutex> lock(m_lock_cnn_wnd);
			auto ite = m_map_cnn_wnd.find(cnn);
			if (ite != m_map_cnn_wnd.end())
				m_map_cnn_wnd.erase(ite);
			m_map_cnn_wnd[cnn] = std::make_shared<md_cli_wnd>();
		}
		void NetTcpClient::del_cnn_wnd(const TcpConnectionPtr& cnn)
		{
			std::lock_guard<std::mutex> lock(m_lock_cnn_wnd);
			auto ite = m_map_cnn_wnd.find(cnn);
			if (ite != m_map_cnn_wnd.end())
			{
				m_map_cnn_wnd.erase(ite);
			}
		}

		int NetTcpClient::send(std::string data)
		{
			TcpConnectionPtr c = get_next_cnn();
			if (c && c->connected())
			{
				if (false == update_cnn_wnd(c))
					return -100;

				c->send(data, nullptr);

			}
			else
				return -1;
			return 0;

		}
		int NetTcpClient::send(const char* data, size_t len)
		{
			TcpConnectionPtr c = get_next_cnn();
			if (c && c->connected())
			{
				if (false == update_cnn_wnd(c))
					return -100;

				c->send(data, static_cast<int>(len), nullptr);
			}
			else
				return -1;
			return 0;

		}

		// ��ȡ��ǰ����״̬
		bool NetTcpClient::state()
		{
			std::lock_guard<std::mutex> lock(m_lock_cnn_wnd);
			for (auto& item : m_map_cnn_wnd)
			{
				auto& cnn_spr = item.first;
				if (cnn_spr
					&& cnn_spr->connected())
				{
					return true;
				}
			}
			return false;
		}
		// ��ȡ��ǰ���ջ����С
		size_t NetTcpClient::recvbuf_size()
		{
			return m_nCurRecvBuf;
		}
		// ��ȡ��ǰ���ڴ�С
		size_t NetTcpClient::cur_wnd_size()
		{
			size_t wndsize = 0;
			std::lock_guard<std::mutex> lock(m_lock_cnn_wnd);
			for (auto& item : m_map_cnn_wnd)
			{
				auto cnn_wnd_spr = item.second;
				wndsize += cnn_wnd_spr->total();
			}
			return wndsize;
		}



		//////////////////////////////////////////////////////////////////////////
		//////////////////////////////////////////////////////////////////////////
		class MwTcpClientCallback;
		MwCli::MwCli()
			: m_pInvokePtr(NULL)
			, m_clients(static_cast<int>(4))
			, m_nIdelTime(0)
		{}
		MwCli::~MwCli()
		{}
		MwCli& MwCli::instance()
		{
			static MwCli cli;
			return cli;
		}


		//�ص��ӿ�
// 		typedef void(*CallBackRecv)(const void* /* pInvokePtr */, 
// 			int /* nLinkId */,
// 			int /* command // 0���ӡ�1���͡�2���� */, 
// 			int /* ret // true�ɹ�, ����ʧ�� */, 
// 			const char* /* data */, 
// 			size_t /* len */,
// 			int& /* nReadedLen */);
		std::mutex g_mwtcp_lock;
		void MwTcpClientCallback::set_mwcli_spr(std::shared_ptr<MwCli> mwcli_spr)
		{
			m_mwcli_spr = mwcli_spr;
		}
		void MwTcpClientCallback::on_cnn(const void* pInvokePtr, int64_t uuid, int nNodeNum,
			const boost::any& conn, const boost::any& param, int ret, std::string info)
		{
			int nReadedLen = 0;
			if (m_mwcli_spr && m_mwcli_spr->m_mw_cb)
			{
				// ֪ͨ
				if (m_mwcli_spr->m_params.bMayUseBlockOpration)
				{
					std::lock_guard<std::mutex> lock(g_mwtcp_lock);
					auto ite = m_mwcli_spr->m_map_index_cli_spr.find(nNodeNum);
					if (ite != m_mwcli_spr->m_map_index_cli_spr.end())
					{
						if (ite->second && 1 == ite->second->moder)
						{
							std::unique_lock<std::mutex> lock_(ite->second->m_notify_lock);
							ite->second->m_notify.notify_one();
						}
					}
				}

				m_mwcli_spr->m_mw_cb(pInvokePtr, nNodeNum, 0, ret, "", 0, nReadedLen);
			}
		}

		void MwTcpClientCallback::on_send(const void* pInvokePtr, int64_t uuid, int nNodeNum,
			const boost::any& conn, const boost::any& param, std::shared_ptr<NetTcpClientInterface> c, size_t remain_len)
		{
			int nReadedLen = 0;
			std::weak_ptr<NetTcpClientInterface> weakcli(c);
			if (m_mwcli_spr && m_mwcli_spr->m_mw_cb)
				m_mwcli_spr->m_mw_cb(pInvokePtr, nNodeNum, 1, 0, "", 0, nReadedLen);
		}

		int MwTcpClientCallback::on_recv(const void* pInvokePtr, int64_t uuid, int nNodeNum,
			const boost::any& conn, const boost::any& param, std::shared_ptr<NetTcpClientInterface> c,
			const char* data, size_t len, int& nReadedLen, int64_t cur_time)
		{
			// nReadedLen = len;

			std::weak_ptr<NetTcpClientInterface> weakcli(c);
			if (m_mwcli_spr && m_mwcli_spr->m_mw_cb)
				m_mwcli_spr->m_mw_cb(pInvokePtr, nNodeNum, 2, 0, data, len, nReadedLen);

			return 0;
		}
		//////////////////////////////////////////////////////////////////////////


		



	}
}