#ifndef MWNET_MT_TCPCLIENT_H
#define MWNET_MT_TCPCLIENT_H

#include <stdint.h>
#include <stdio.h>
#include <string>
#include <memory>
#include <atomic>
#include <mutex>
#include <boost/any.hpp>

namespace MWNET_MT
{
namespace CLIENT
{
// 网络连接或断开回调函数
//返回值0:代表成功 >0代表其他各种错误或反控等信息(待定) <0:底层会主动强制关闭连接
typedef int(*pfunc_on_connection)(void* pInvoker, uint64_t conn_uuid, bool bConnected, const char* szIpPort, const char* szIp, uint16_t port);

// 网络收到数据回调函数
//返回值0:代表成功 >0代表其他各种错误或反控等信息(待定) <0:底层会主动强制关闭连接
typedef int(*pfunc_on_readmsg_tcpclient)(void* pInvoker, uint64_t conn_uuid, const char* szMsg, int nMsgLen, int& nReadedLen);

// 网络数据发送完成回调函数
//返回值0:代表成功 >0代表其他各种错误或反控等信息(待定) <0:底层会主动强制关闭连接
typedef int(*pfunc_on_sendok)(void* pInvoker, uint64_t conn_uuid, const boost::any& params);

//高水位回调 当底层缓存的待发包数量超过设置的值时会触发该回调
//返回值0:代表成功 >0代表其他各种错误或反控等信息(待定) <0:底层会主动强制关闭连接
typedef int (*pfunc_on_highwatermark)(void* pInvoker, const uint64_t conn_uuid, size_t highWaterMark);

class xTcpClient;
typedef std::shared_ptr<xTcpClient> XTcpClientPtr;

// this netclient only support single connection
class NetClient
{
public:
	NetClient();
	~NetClient();

public:
	// 初始化,初始化完成后将自动建立连接，如果连接断开，请调用ReConnect
	bool InitTcpClient(void* pInvoker,									/*上层调用的类指针*/
						const std::string& ip,								/*服务端IP*/
						uint16_t  port,										/*服务端IP*/
						pfunc_on_connection pOnConnection,					/*建立连接或连接断开回调*/
						pfunc_on_readmsg_tcpclient pOnReadMsg,				/*网络收到数据回调函数*/
						pfunc_on_sendok pOnSendOk,							/*网络数据发送完成回调函数*/
						pfunc_on_highwatermark pOnHighWaterMark=NULL,		/*高水位回调*/
						size_t nDefRecvBuf=1*1024*1024,						/*默认接收buf*/
						size_t nMaxRecvBuf=4*1024*1024,						/*最大接收buf*/
						size_t nMaxSendQue=1024								/*最大发送队列*/
						);

	// 设置高水位值
	void SetHighWaterMark(size_t highwatermark);
	
	// 发送请求
	// 0:成功 非0:失败
	int  SendTcpRequest(const boost::any& params, const char* szMsg, size_t nMsgLen, bool bKeepAlive=true);

	// 重新连接
	void  ReConnect(const std::string& ip, uint16_t port);

	int64_t getConnuuid();

	std::string getIpPort();
	
	// 断开连接
	void  DisConnect();
private:
	void  Connect();
	void  ResetClient(XTcpClientPtr& xclient);
	void  ResetClientInLoop(XTcpClientPtr& xclient);
private:
	void* m_pInvoker;
	size_t m_nDefRecvBuf;
	size_t m_nMaxRecvBuf;
	size_t m_nMaxSendQue;
	pfunc_on_connection m_pfunc_on_connection;
	pfunc_on_readmsg_tcpclient m_pfunc_on_readmsg_tcp;
	pfunc_on_sendok m_pfunc_on_sendok;
	pfunc_on_highwatermark m_pfunc_on_highwatermark;
	std::mutex lock_;
	// tcpclient
	XTcpClientPtr client_;
	// mainloop
	boost::any main_loop_;
};
}
}

#endif
