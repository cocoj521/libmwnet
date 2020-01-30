#ifndef MWNET_MT_TCPCLIENT_H
#define MWNET_MT_TCPCLIENT_H

#include <stdint.h>
#include <stdio.h>
#include <string>
#include <memory>
#include <mutex>
#include <map>
#include <boost/any.hpp>

namespace MWNET_MT
{
namespace TCPCLIENT
{
	// 连接状态
	enum enumConnStatus
	{
		CONNSTATUS_INIT = 0,			// 初始状态
		CONNSTATUS_CONNECTING = 1,		// 连接中
		CONNSTATUS_CONNECTED = 2,		// 连接成功
		CONNSTATUS_CONNECTFAIL = 3,		// 连接失败
		CONNSTATUS_CONNECTTIMEOUT = 4,	// 连接超时
		CONNSTATUS_DISCONNECT = 5		// 连接断开
	};

	// 建立连接所需的基本信息
	struct connection_baseinfo_t
	{
		std::string conn_ip;				// (IN)连接Ip地址或域名
		uint16_t conn_port;					// (IN)连接端口
		uint16_t conn_timeout;				// (IN)连接建立超时间(s)
		boost::any any_data;				// (IN)任意数据

		////////////////////////////////////////////////////////////////
		//以下字段在建连回调后才会返回,建连时不要填写,回调返回后也不可以改变
		uint64_t conn_uuid;					// (OUT)连接惟一ID,连接建立成功后才会被赋值
		boost::any conn_obj;				// (OUT)连接对象,连接建立成功后才会被赋值
		uint8_t  conn_status;				// (OUT)连接状态 取值参见enumConnStatus
		std::string conn_status_desc;		// (OUT)连接状态描述
		int64_t conn_begin_tm_ms;			// (OUT)建连开始时间(ms)
		int64_t conn_end_tm_ms;				// (OUT)建连结束时间(ms)

		connection_baseinfo_t()
		{
			conn_status = CONNSTATUS_INIT;
			conn_timeout = 3;
		}
	};
	typedef std::shared_ptr<connection_baseinfo_t> ConnBaseInfoPtr;

	//////////////////////////////////////////////////////回调函数//////////////////////////////////////////////////////////////
	//上层调用代码在实现时，必须要快速操作，不可以加锁，否则将阻塞底层处理，尤其要注意，不能加锁，否则底层多线程将变成单线程

	//与连接相关的回调(连接/关闭等)
	//返回值0:代表成功 >0代表其他各种错误或反控等信息(待定) <0:底层会主动强制关闭连接
	//sessioninfo为调用settcpsessioninfo,sethttpsessioninfo时传入的sessioninfo
	typedef int(*pfunc_on_connection)(void* pInvoker, const ConnBaseInfoPtr& pConnBaseInfo, bool bConnected, const char* szIpPort);

	//从网络层读取到信息的回调(适用于tcpserver)
	//返回值0:代表成功 >0代表其他各种错误或反控等信息(待定) <0:底层会主动强制关闭连接
	//nReadedLen用于控制残包粘包，如果是残包:必须填0，如果是粘包:必须填完整包的大小*完整包的个数
	//sessioninfo为调用settcpsessioninfo,sethttpsessioninfo时传入的sessioninfo
	typedef int(*pfunc_on_readmsg_tcp)(void* pInvoker, const ConnBaseInfoPtr& pConnBaseInfo, const char* szMsg, int nMsgLen, int& nReadedLen);

	//发送数据完成的回调
	//返回值0:代表成功 >0代表其他各种错误或反控等信息(待定) <0:底层会主动强制关闭连接
	//params为调用sendhttpmsg,sendtcpmsg时传入的params
	//sessioninfo为调用settcpsessioninfo,sethttpsessioninfo时传入的sessioninfo
	typedef int(*pfunc_on_sendok)(void* pInvoker, const ConnBaseInfoPtr& pConnBaseInfo, const boost::any& params);

	// 发送数据失败的回调,参数意义与pfunc_on_sendok相同
	// errCode:
	//1:连接已不可用        
	//2:发送缓冲满(暂时不会返回,暂时通过高水位回调告知上层)       
	//3:调用write函数失败 
	//4:发送超时(未在设置的超时间内发送成功)
	typedef int(*pfunc_on_senderr)(void* pInvoker, const ConnBaseInfoPtr& pConnBaseInfo, const boost::any& params, int errCode);

	//高水位回调 当底层缓存的待发包数量超过设置的值时会触发该回调
	//返回值0:代表成功 >0代表其他各种错误或反控等信息(待定) <0:底层会主动强制关闭连接
	typedef int(*pfunc_on_highwatermark)(void* pInvoker, const ConnBaseInfoPtr& pConnBaseInfo, size_t highWaterMark);
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class mTcpClient:
	public std::enable_shared_from_this<mTcpClient>
{
	static const size_t kDefRecvBuf = 32 * 1024;
	static const size_t kMaxRecvBuf = 1* 1024 * 1024;
	static const size_t kMaxSendQue = 2048;
	static const uint16_t kIoThrNum = 4;
	static const uint16_t kConnectTimeOut = 3;

public:
	mTcpClient();
	~mTcpClient();

public:
	// 初始化
	bool InitTcpClient(void* pInvoker,										// 上层调用的类指针
						pfunc_on_connection pOnConnection,					// 建立连接或连接断开回调
						pfunc_on_readmsg_tcp pOnReadMsg,					// 网络收到数据回调函数
						pfunc_on_sendok pOnSendOk,							// 网络数据发送完成回调函数
						pfunc_on_senderr pOnSendErr,						// 发送数据失败的事件回调
						pfunc_on_highwatermark pOnHighWaterMark=NULL,		// 高水位回调
						uint16_t threadnum = kIoThrNum,						// IO线程数
						size_t nDefRecvBuf = kDefRecvBuf,					// 默认接收buf
						size_t nMaxRecvBuf = kMaxRecvBuf,					// 最大接收buf
						size_t nMaxSendQue = kMaxSendQue					// 最大发送队列
						);
	
	// 反初始化
	void UnInitTcpClient();

	// 添加连接,每个连接信息都必须是不同的智能指针,不可以相同
	// 该函数是异步的,结果通过回调函数返回
	void AddConnection(const ConnBaseInfoPtr& pConnBaseInfo);

	// 删除连接/断开连接
	// 该函数是异步的,结果通过回调函数返回
	void DelConnection(const ConnBaseInfoPtr& pConnBaseInfo);

	// 发送消息 
	// params:自定义发送参数，发送完成/失败回调时会返回，请用智能指针
	// szMsg,nMsgLen:要发送数据的内容和长度
	// timeout:要求底层多长时间内必须发出，若未发送成功返回失败的回调.单位:毫秒,默认为0,代表不要求超时间
	// bKeepAlive:发完以后是否保持连接.若设置为false，当OnSendOk回调完成后，会自动断开连接
	//0:成功    1:当前连接已不可用          2:当前连接最大发送缓冲已满
	int  SendMsgWithTcpClient(const ConnBaseInfoPtr& pConnBaseInfo, const boost::any& params, const char* szMsg, size_t nMsgLen, int timeout_ms = 0, bool bKeepAlive = true);

	// 设置高水位值
	// 建连成功以后才可以调用
	void SetHighWaterMark(const ConnBaseInfoPtr& pConnBaseInfo, size_t highwatermark);

	// 总连接数
	size_t GetTotalConnCnt() const;

	//暂停当前TCP连接的数据接收
	//delay:暂停接收的时长.单位:毫秒,默认为10ms,<=0时不延时
	void SuspendTcpConnRecv(const ConnBaseInfoPtr& pConnBaseInfo, int delay);

	//恢复当前TCP连接的数据接收
	void ResumeTcpConnRecv(const ConnBaseInfoPtr& pConnBaseInfo);
private:
	void AddConnToMap(const ConnBaseInfoPtr& pConnBaseInfo);
	bool DelConnFromMap(const ConnBaseInfoPtr& pConnBaseInfo);
	void ClearConnMap();
	void AddConnectionInLoop(const ConnBaseInfoPtr& pConnBaseInfo);
	void DelConnectionInLoop(const ConnBaseInfoPtr& pConnBaseInfo);
private:
	void* m_pInvoker;
	size_t m_nDefRecvBuf;
	size_t m_nMaxRecvBuf;
	size_t m_nMaxSendQue;
	uint16_t m_nIoThrNum;
	pfunc_on_connection m_pfunc_on_connection;
	pfunc_on_readmsg_tcp m_pfunc_on_readmsg_tcp;
	pfunc_on_sendok m_pfunc_on_sendok;
	pfunc_on_senderr m_pfunc_on_senderr;
	pfunc_on_highwatermark m_pfunc_on_highwatermark;

	std::mutex lock_;
	// all connections
	std::map<uint64_t, ConnBaseInfoPtr> connections_;
	// mainloop
	boost::any main_loop_;
	boost::any io_thread_pool_;
	bool exit_;
};
}
}

#endif
