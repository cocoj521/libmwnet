#ifndef MWNET_MT_HTTPCLIENT_H
#define MWNET_MT_HTTPCLIENT_H

#include "NetClient.h"

#include <mwnet_mt/net/TcpClient.h>
#include <mwnet_mt/net/Buffer.h>
#include <mwnet_mt/net/EventLoopThreadPool.h>
#include <stdint.h>
#include <stdio.h>
#include <boost/any.hpp>
#include <string>
#include <memory>
#include <functional>
#include <mwnet_mt/base/Types.h>
#include <map>
#include <set>
#include <time.h>
#include <iostream>

using namespace mwnet_mt;
using namespace mwnet_mt::net;

namespace MWNET_MT
{
namespace CLIENT
{

#define  ENTER "\r\n"

class HttpClient;

class HttpClientManager
{
	typedef std::shared_ptr<HttpClient> HttpClientPtr;
public:
	HttpClientManager();

	void Init(
		void* pInvoker, 
		int nThreadNum, 
		int idleconntime, 
		int nMaxHttpClient, 
		pfunc_on_msg_httpclient pFn,
		size_t nDefRecvBuf,
		size_t nMaxRecvBuf,
		size_t nMaxSendQue);

	void Exit();

	// 发送请求
	int Http_Request(uint64_t req_uuid, const boost::any& params, const HttpRequest& httpRequest);

	//添加client
	int Add_HttpClient(uint64_t client_uuid, const HttpClientPtr& httpClientPtr, const std::string& strKey = "", bool bKeepLive = false);

	//删除client
	int Del_HttpClient(uint64_t client_uuid);

	int Del_HttpClientEx(uint64_t client_uuid);

	//client是否可用
	bool IsConnectd(uint64_t client_uuid);

	//检查当前并发是否可用
	bool IsCheckMaxRequest();

	// 检测超时连接并清理
	void OnCheckTimeOut();

	EventLoop* getLoop();

private:
	int Request_Asyn(uint64_t req_uuid, const boost::any& params, const HttpRequest& httpRequest);

	//选择长连接发送
	int Request_LongConn(uint64_t req_uuid, const HttpRequest& httpRequest, const boost::any& params, const std::string strKey, bool bKeepLive = true);

	int Request_ShortConn(uint64_t req_uuid, const HttpRequest& httpRequest, const boost::any& params);

	//生成httpclient唯一的ID
	uint64_t GetNewUuid(){ return m_llUUIDSource++; };

	void start();

	void OnExit();

private:
	typedef std::map<std::string/*IP_PORT*/, std::set<uint64_t>> HttpKeepAliveInfoMap;
	typedef std::map<uint64_t, HttpClientPtr> HttpClientMap;
	
	std::shared_ptr<EventLoopThreadPool> threadPool_;
	MutexLock							 m_csLock;
	HttpKeepAliveInfoMap				 httpKeepAliveInfoMap_; //长连接映射
	HttpClientMap						 httpClientMap_;
	
	uint64_t							 m_llUUIDSource;
	bool								 m_bInit;
	EventLoop							 *loop_;
	int									 m_nIdelTime;
	void								 *m_pInvoker;
	int									 m_nMaxHttpClient; //最大并发数
	pfunc_on_msg_httpclient				 pFnOnMessage;	   //消息回调函数

	size_t								 m_nDefRecvBuf;
	size_t								 m_nMaxRecvBuf;
	size_t								 m_nMaxSendQue;
};

class HttpClient
{
public:
	
	HttpClient(HttpClientManager *phttpMgr, 
		EventLoop* loopBase, 
		EventLoop* loopRun, 
		void* pInvoker, 
		uint64_t client_uuid, 
		uint64_t req_uuid, 
		const HttpRequest& httpRequest, 
		const boost::any& conn, 
		pfunc_on_msg_httpclient pFn,
		size_t nDefRecvBuf,
		size_t nMaxRecvBuf,
		size_t nMaxSendQue);

	~HttpClient() {/* std::cout<<"~HttpClient() "<<this<<std::endl; */};

	void send(uint64_t req_uuid, const boost::any& params, const HttpRequest& httpRequest);

	bool isConnected();

	void QuitConnect();

public:

	bool onTimeOut();

	//获取请求IP
	std::string GetIp() const { return m_strIp; }

	//获取Port
	int GetPort()const { return m_nPort; }

	bool ConnectStatus() { return m_bConnected; }

	//是否为长连接
	bool IsKeepAlive() const { return m_bIsKeepAlive; }

	//KEY: IP_PORT
	std::string GetKey() const { return m_strKey; }

private:

	void DealHttpResponse(const TcpConnectionPtr& conn, Buffer* buf, Timestamp time);

	void onConnection(const TcpConnectionPtr& conn);

	void onMessage(const TcpConnectionPtr&conn, Buffer* buf, Timestamp tmstamp);	

private:
	typedef std::shared_ptr<HttpResponse> HttpResponsePtr;

	std::shared_ptr<TcpClient>	client_;		 //tcpClient		
	HttpResponsePtr		m_pHttpResponse; //HTTP回应类
	HttpRequest			m_httpRequest;	 //HTTP请求类
	pfunc_on_msg_httpclient		m_pfnOnMessage;	 //回调函数
	boost::any					m_Params;		 //请求所带参数	
	void*						m_pInvoker;		 
	EventLoop*					m_loopBase;		 //基类IO
	EventLoop*					m_loopRun;		 //IO线程
	HttpClientManager			*m_phttpMgr;	 //http管理类

	bool						m_bFlage;		 //正常请求-响应（完成标志）
	bool						m_bConnected;	 //连接可用标志
	bool						m_bIsKeepAlive;  //长连接标识
	uint64_t					m_llUuid;		 //client在map中KEY
	uint64_t					m_reqUuid;		 //本次请求的ID
	std::string					m_strIp;		 //IP
	std::string					m_strKey;		 //IP_PORT
	int							m_nPort;
	time_t						m_tBengin;		 //发送请求时间
	time_t						m_tEnd;			 //接收回应时间

	size_t			m_nDefRecvBuf;
	size_t			m_nMaxRecvBuf;
	size_t			m_nMaxSendQue;
};

}
}

#endif