#ifndef __NetTcpClient___H__
#define __NetTcpClient___H__


#include <string>
#include <memory>
#include <map>
#include <condition_variable>
#include <boost/any.hpp>

#include <mwnet_mt/base/Atomic.h>
#include <sys/time.h>
using namespace mwnet_mt;

#include "NetClient.h"
#define PRINT_LOG 0
namespace MWNET_MT
{
	namespace CLIENT
	{

		class NetTcpClientInterface;
		class NetTcpClientCallbackInterface
		{
		public:
			virtual void on_cnn(const void* pInvokePtr, int64_t uuid, int nNodeNum, 
				const boost::any& conn, const boost::any& param, int ret, std::string info) = 0;

			// ��������ȡ����Ϣ�Ļص�
			// ����ֵ0:����ɹ� >0�����������ִ���򷴿ص���Ϣ(����) <0:�ײ������ǿ�ƹر�����
			// nReadedLen���ڿ��Ʋа�ճ��������ǲа�:������0�������ճ��:�������������Ĵ�С*�������ĸ���
			virtual int on_recv(const void* pInvokePtr, int64_t uuid, int nNodeNum, 
				const boost::any& conn, const boost::any& param, std::shared_ptr<NetTcpClientInterface> c,
				const char* data, size_t len, int& nReadedLen, int64_t cur_time) = 0;
			virtual void on_send(const void* pInvokePtr, int64_t uuid, int nNodeNum, 
				const boost::any& conn, const boost::any& param, std::shared_ptr<NetTcpClientInterface> c, size_t remain_len) = 0;
		};

		class NetTcpClientInterface
		{
		public:
			virtual int set_callback(const void* pInvokePtr, std::shared_ptr<NetTcpClientCallbackInterface> callback) = 0;
			/*
			Ĭ�ϲ��Զ�����
			*/
			virtual int set_auto_reconnect(bool flag = true) = 0;
			/*
			���ӽ����ɹ�֮����ܵ��ã�
			Ĭ��tcp_delay
			*/
			virtual int set_tcp_delay(bool flag = true) = 0;

			virtual int conn(std::string ip, uint16_t port) = 0;
			virtual int stop() = 0;

			virtual int send(const boost::any& conn, std::string data) = 0;
			virtual int send(const boost::any& conn, const char* data, size_t len) = 0;

			virtual int send(int64_t uuid, const boost::any& param, const char* data, size_t len) = 0;

			virtual int send(std::string data) = 0;
			virtual int send(const char* data, size_t len) = 0;

			virtual int get_node_num() = 0;
			virtual void set_node_num(int nNodeNum) = 0;

			virtual void	setConnuuid(int64_t conn_uuid) = 0;
			virtual int64_t getConnuuid() const = 0;

			virtual void set_buf_info(size_t nDefRecvBuf = 4 * 1024,		/*Ĭ�Ͻ���buf*/
				size_t nMaxRecvBuf = 4 * 1024,		/*������buf*/
				size_t nMaxSendQue = 512			/*����Ͷ���*/) = 0;

			// ��ȡ��ǰ����״̬ nNodeNum = -1, ȡ����
			virtual bool state() = 0;
			// ��ȡ��ǰ���ջ����С
			virtual size_t recvbuf_size() = 0;
			// ��ȡ��ǰ���ڴ�С
			virtual size_t cur_wnd_size() = 0;

		};


		//////////////////////////////////////////////////////////////////////////
		//////////////////////////////////////////////////////////////////////////

		std::shared_ptr<NetTcpClientInterface> get_md_cli(uint16_t clients = 5, size_t wnd_size = 100);
		int md_cli_init(int thread_num);
		int md_cli_uninit();

		//////////////////////////////////////////////////////////////////////////

		class NetTcpClientMgrCallback;
		class NetTcpClientMgr
		{
		public:
			NetTcpClientMgr();
			~NetTcpClientMgr();

			uint16_t m_port;
			std::string m_ip;
			const void* m_pInvokePtr;

			int m_clients;
			int m_wnd_size;
			int m_nIdelTime;

			size_t m_nDefRecvBuf;
			size_t m_nMaxRecvBuf;
			size_t m_nMaxSendQue;

			std::shared_ptr<NetTcpClientInterface> m_cli_spr;

			std::shared_ptr<NetTcpClientMgrCallback> m_cb_spr;

			static NetTcpClientMgr& instance();

		};

		uint64_t MakeConnuuid(uint16_t unique_node_id = 1008);
		class NetTcpClientMgrContaner
		{
		public:
			NetTcpClientMgrContaner();
			~NetTcpClientMgrContaner();

			pfunc_on_readmsg_tcpclient m_tcpclient_readmsg_cb;
			pfunc_on_connection	m_tcpclient_connect_cb;
			pfunc_on_sendok m_tcpclient_send_cb;

			uint64_t add(std::shared_ptr<NetTcpClientMgr> item);
			void del(std::shared_ptr<NetTcpClientMgr> item);
			void del(uint64_t uuid);
			std::shared_ptr<NetTcpClientMgr> get(uint64_t uuid);
			std::shared_ptr<NetTcpClientInterface> get_cli(uint64_t uuid, uint64_t cnn_uuid);
			bool empty();

			static NetTcpClientMgrContaner& instance()
			{
				static NetTcpClientMgrContaner inst_;
				return inst_;
			}
		private:
			std::map<uint64_t, std::shared_ptr<NetTcpClientMgr>> m_map_mgr;
			std::mutex m_lock;

			

		};

		class NetTcpClientMgrCallback
			: public NetTcpClientCallbackInterface
		{
		public:
			void on_cnn(const void* pInvokePtr, int64_t uuid, int nNodeNum, 
				const boost::any& conn, const boost::any& param, int ret, std::string info);

			void on_send(const void* pInvokePtr, int64_t uuid, int nNodeNum, 
				const boost::any& conn, const boost::any& param, std::shared_ptr<NetTcpClientInterface> c, size_t remain_len);

			int on_recv(const void* pInvokePtr, int64_t uuid, int nNodeNum, 
				const boost::any& conn, const boost::any& param, std::shared_ptr<NetTcpClientInterface> c,
				const char* data, size_t len, int& nReadedLen, int64_t cur_time);
		};


		/*
void def_rms_cb(const void* pInvokePtr,
	int nNodeNum,			// �ڵ�
	const char* command,	// on_conn���ӡ�on_send���͡�on_recv����
	int ret,				// true�ɹ�, ����ʧ��
	const char* data,
	size_t len,
	int& nReadedLen,
	int64_t cur_time)
		*/

		// �����޸�
		typedef void(*RmsCb)(const void*, \
			int, \
			const char*, \
			int, \
			const char*, \
			size_t, \
			int&, \
			int64_t);

		class RmsTcpClientCallback;
		class RmsCli
		{
		public:
			RmsCli();
			~RmsCli();

			const void* m_pInvokePtr;

			int m_clients;
			int m_wnd_size;
			int m_nIdelTime;

			size_t m_nDefRecvBuf;
			size_t m_nMaxRecvBuf;
			size_t m_nMaxSendQue;

			std::map<int, std::shared_ptr<NetTcpClientInterface>> m_map_index_cli_spr;

			RmsCb m_rms_cb;

			std::shared_ptr<RmsTcpClientCallback> m_cb_spr;

			static RmsCli& instance();

		};

		class RmsTcpClientCallback
			: public NetTcpClientCallbackInterface
		{
		public:
			void on_cnn(const void* pInvokePtr, int64_t uuid, int nNodeNum, 
				const boost::any& conn, const boost::any& param, int ret, std::string info);

			void on_send(const void* pInvokePtr, int64_t uuid, int nNodeNum, 
				const boost::any& conn, const boost::any& param, std::shared_ptr<NetTcpClientInterface> c, size_t remain_len);

			int on_recv(const void* pInvokePtr, int64_t uuid, int nNodeNum, 
				const boost::any& conn, const boost::any& param, std::shared_ptr<NetTcpClientInterface> c,
				const char* data, size_t len, int& nReadedLen, int64_t cur_time);
		};


		//////////////////////////////////////////////////////////////////////////
#define MAX_IPADDR_LEN 18
		struct RMS_LINKNODE
		{
			int nNodeNum;//�ڵ��� ע���ڵ���Ψһ
			int nPort; //�˿�
			char szIp[MAX_IPADDR_LEN + 1]; //ip  ע��MAX_IPADDR_LEN=18  

			RMS_LINKNODE()
			{
				memset(this, 0, sizeof(RMS_LINKNODE));
			}
		};

		struct RMS_PARAMS
		{
			int iClientsPerNode;
			std::vector<RMS_LINKNODE> vecLinkNode;

		};

		/*
		���ܣ� ��ʼ��
		pInvokePtr �û��Զ�������
		thread_num �߳���
		wnd_size ������С
		*/
		int RmsInitialise(const void* pInvokePtr, RmsCb cb, RMS_PARAMS& rms_param, 
			int thread_num = 5,
			size_t wnd_size = 30,
			size_t nDefRecvBuf = 4 * 1024,		/*Ĭ�Ͻ���buf*/
			size_t nMaxRecvBuf = 4 * 1024,		/*������buf*/
			size_t nMaxSendQue = 512			/*����Ͷ���*/);

		/*
		���ܣ� ֹͣ
		*/
		int RmsStop();

		/*
		���ܣ� ��ֹ��
		��ҪRmsStop�󣬵ȴ����������ϵ��ٵ��ã�����Ҫ�Լ������׳����쳣
		pInvokePtr �û��Զ�������
		thread_num �߳���(���ڲ����޸�)
		*/
		int RmsUnInitialise();

		/*
		���ܣ� ������Ϣ
		nNodeNum �ڵ���
		szData	����
		nLen	���ݳ���
		*/
		int RmsSend(int nNodeNum, const char *szData, const int nLen);

		//////////////////////////////////////////////////////////////////////////


		//////////////////////////////////////////////////////////////////////////
		//////////////////////////////////////////////////////////////////////////
		


		class MwTcpClientCallback;
		class MwCli
		{
		public:
			MwCli();
			~MwCli();

			struct mwcli_info
			{
				mwcli_info()
					: port(0)
					, nLinkId(0)
					, moder(0)
					, ms_timeout(0)
				{}
				mwcli_info(std::shared_ptr<NetTcpClientInterface> cli_spr_,
					int nLinkId_, const std::string& ip_, int port_
					, int moder_, int ms_timeout_)
					: cli_spr(cli_spr_)
					, ip(ip_)
					, port(port_)
					, nLinkId(nLinkId_)
					, moder(moder_)
					, ms_timeout(ms_timeout_)
				{}
				std::shared_ptr<NetTcpClientInterface> cli_spr;
				std::string ip;
				int port;
				int nLinkId;
				int moder;
				int ms_timeout;
				std::mutex m_notify_lock;
				std::condition_variable m_notify;
			};

			const void* m_pInvokePtr;

			int m_clients;
			int m_nIdelTime;
			std::map<int, std::shared_ptr<mwcli_info>> m_map_index_cli_spr;

			CallBackRecv m_mw_cb;
			Params m_params;

			std::shared_ptr<MwTcpClientCallback> m_cb_spr;

			static MwCli& instance();

		};

		class MwTcpClientCallback
			: public NetTcpClientCallbackInterface
		{
		public:
			void on_cnn(const void* pInvokePtr, int64_t uuid, int nNodeNum,
				const boost::any& conn, const boost::any& param, int ret, std::string info);

			void on_send(const void* pInvokePtr, int64_t uuid, int nNodeNum,
				const boost::any& conn, const boost::any& param, std::shared_ptr<NetTcpClientInterface> c, size_t remain_len);

			int on_recv(const void* pInvokePtr, int64_t uuid, int nNodeNum,
				const boost::any& conn, const boost::any& param, std::shared_ptr<NetTcpClientInterface> c,
				const char* data, size_t len, int& nReadedLen, int64_t cur_time);

			void set_mwcli_spr(std::shared_ptr<MwCli> mwcli_spr);
		protected:
			std::shared_ptr<MwCli> m_mwcli_spr;
		};

		//////////////////////////////////////////////////////////////////////////
		//////////////////////////////////////////////////////////////////////////
		
	}
}

#endif
