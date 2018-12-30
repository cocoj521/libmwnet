#include "httpclient.h"

#include <sstream>
#include <stdio.h>
#include <mwnet_mt/base/ThreadLocalSingleton.h>
#include <mwnet_mt/net/EventLoopThreadPool.h>
#include <mwnet_mt/net/EventLoop.h>
#include <mwnet_mt/net/httpparser/httpparser.h>

using namespace mw_http_parser;
using namespace mwnet_mt;
using namespace mwnet_mt::net;

namespace MWNET_MT{
namespace CLIENT{

HttpClientManager::HttpClientManager()
{
	m_bInit = false;
	threadPool_ = nullptr;
	pFnOnMessage = nullptr;
	loop_ = nullptr;
	m_nIdelTime = 3;
	m_pInvoker = nullptr;
	m_nMaxHttpClient = 100;
	m_llUUIDSource = 0;

	m_nDefRecvBuf = 1024 * 16;
	m_nMaxRecvBuf = 1024 * 64;
	m_nMaxSendQue = 100;
}

void HttpClientManager::Init(
	void* pInvoker, 
	int nThreadNum, 
	int nIdleTime, 
	int nMaxHttpClient, 
	pfunc_on_msg_httpclient pFn,
	size_t nDefRecvBuf,
	size_t nMaxRecvBuf,
	size_t nMaxSendQue)
{
	if (!loop_ && !m_bInit)
	{
		loop_ = new EventLoop;
		pFnOnMessage = pFn;
		m_nIdelTime = nIdleTime;
		m_pInvoker = pInvoker;
		m_nMaxHttpClient = nMaxHttpClient;

		m_nDefRecvBuf = nDefRecvBuf;
		m_nMaxRecvBuf = nMaxRecvBuf;
		m_nMaxSendQue = nMaxSendQue;

		if (!threadPool_)
		{
			threadPool_.reset(new EventLoopThreadPool(loop_, "ThreadPool"));
			if (threadPool_)
			{
				threadPool_->setThreadNum(nThreadNum);
				loop_->runInLoop(std::bind(&HttpClientManager::start, this));
				m_bInit = true;
			}
		}
		loop_->runEvery(1, std::bind(&HttpClientManager::OnCheckTimeOut, this));
		loop_->loop();
	}
}

void HttpClientManager::OnExit()
{
	m_bInit = false;
	if (loop_)
	{
		while (httpClientMap_.size())
		{
			sleep(1);
		}
		loop_->quit();
	}
}

void HttpClientManager::Exit()
{
	OnExit();
	//loop_->runInLoop(std::bind(&HttpClientManager::OnExit, this));
}

EventLoop* HttpClientManager::getLoop()
{ 
	return threadPool_->getNextLoop(); 
}

void HttpClientManager::start()
{
	threadPool_->start();
	m_bInit = true;
}

int HttpClientManager::Add_HttpClient(uint64_t client_uuid, const HttpClientPtr& httpClientPtr, const std::string& strKey, bool bKeepLive)
{
	//长连接
	if (bKeepLive)
	{
		if (!strKey.empty())
		{
			HttpKeepAliveInfoMap::iterator it = httpKeepAliveInfoMap_.find(strKey);
			if (it != httpKeepAliveInfoMap_.end())
			{
				it->second.insert(client_uuid);
			}
			else
			{
				std::set<uint64_t> vSet;
				vSet.insert(client_uuid);

				httpKeepAliveInfoMap_.insert(std::make_pair(strKey, vSet));
			}
			httpClientMap_.insert(std::make_pair(client_uuid, httpClientPtr));
		}	
	}
	else
	{
		httpClientMap_.insert(std::make_pair(client_uuid, httpClientPtr));
	}

	return 0;
}

int HttpClientManager::Del_HttpClient(uint64_t client_uuid)
{
	MutexLockGuard lock(m_csLock);

	try
	{
		HttpClientMap::iterator it = httpClientMap_.find(client_uuid);

		if (httpClientMap_.end() != it)
		{
			bool bKeepAlive = it->second->IsKeepAlive();
			//长连接清除映射关系
			if (bKeepAlive)
			{
				std::string strKey = it->second->GetKey();

				HttpKeepAliveInfoMap::iterator itKeepAlive = httpKeepAliveInfoMap_.find(strKey);
				if (httpKeepAliveInfoMap_.end() != itKeepAlive)
				{
					itKeepAlive->second.erase(client_uuid);
				}
			}
			httpClientMap_.erase(it);
		}
	}
	catch (...)
	{

	}

	return 0;
}

int HttpClientManager::Del_HttpClientEx(uint64_t client_uuid)
{
	//MutexLockGuard lock(m_csLock);

	try
	{
		HttpClientMap::iterator it = httpClientMap_.find(client_uuid);

		if (httpClientMap_.end() != it)
		{
			bool bKeepAlive = it->second->IsKeepAlive();
			//长连接清除映射关系
			if (bKeepAlive)
			{
				std::string strKey = it->second->GetKey();

				HttpKeepAliveInfoMap::iterator itKeepAlive = httpKeepAliveInfoMap_.find(strKey);
				if (httpKeepAliveInfoMap_.end() != itKeepAlive)
				{
					itKeepAlive->second.erase(client_uuid);
				}
			}
			httpClientMap_.erase(it);
		}
	}
	catch (...)
	{

	}

	return 0;
}
bool HttpClientManager::IsConnectd(uint64_t client_uuid)
{
	MutexLockGuard lock(m_csLock);

	bool bRet = true;

	HttpClientMap::iterator it = httpClientMap_.find(client_uuid);
	if (httpClientMap_.end() != it)
	{
		if (!it->second->isConnected())
		{
			bRet = false;
		}
	}

	return bRet;
}

bool HttpClientManager::IsCheckMaxRequest()
{
	MutexLockGuard lock(m_csLock);
	unsigned long nSize = httpClientMap_.size();
	return (nSize >= static_cast<unsigned int>(m_nMaxHttpClient) ? true : false);
}

void HttpClientManager::OnCheckTimeOut()
{
	MutexLockGuard lock(m_csLock);

	try
	{
		HttpClientMap::iterator it = httpClientMap_.begin();

		for (; it != httpClientMap_.end();)
		{
			if (it->second->onTimeOut())
			{
				if (!it->second->IsKeepAlive() && !it->second->ConnectStatus())
				{
					httpClientMap_.erase(it++);
				}
				else
				{
					++it;
				}
			}
			else
			{
				++it;
			}
		}
	}
	catch (...)
	{
		
	}

}

int HttpClientManager::Http_Request(uint64_t req_uuid, const boost::any& params, const HttpRequest& httpRequest)
{
	int nRet = -1;

	if (loop_ && m_bInit)
	{	
		//检测最大并发量
		if (!IsCheckMaxRequest())
		{
			loop_->runInLoop(std::bind(&HttpClientManager::Request_Asyn, this, req_uuid, params, httpRequest));
			nRet = 0;
		}
	}

	return nRet;
}

int HttpClientManager::Request_LongConn(uint64_t req_uuid, const HttpRequest& httpRequest, const boost::any& params, const std::string strKey, bool bKeepLive /* = true */)
{
	//找映射关系
	HttpKeepAliveInfoMap::iterator it = httpKeepAliveInfoMap_.find(strKey);

	if (it != httpKeepAliveInfoMap_.end())
	{
		std::set<uint64_t>::const_iterator itUuid = it->second.begin();

		// 选择一条可用的长连接
		for (; itUuid != it->second.end(); ++itUuid)
		{
			HttpClientMap::iterator itClient = httpClientMap_.find(*itUuid);
			if (httpClientMap_.end() != itClient)
			{
				if (itClient->second->isConnected())
				{
					itClient->second->send(req_uuid, params,httpRequest);
					return 0;
				}
			}
		}
	}
	
	//无可用连接生成新连接
	uint64_t client_uuid = GetNewUuid();
	HttpClientPtr clientPtr(new HttpClient(this, loop_, getLoop(), m_pInvoker, client_uuid,req_uuid, httpRequest, params, pFnOnMessage,m_nDefRecvBuf, m_nMaxRecvBuf, m_nMaxSendQue));
	Add_HttpClient(client_uuid, clientPtr, strKey, bKeepLive);

	return 0;
}

int HttpClientManager::Request_ShortConn(uint64_t req_uuid, const HttpRequest& httpRequest, const boost::any& params)
{
	uint64_t client_uuid = GetNewUuid();
	HttpClientPtr clientPtr(new HttpClient(this, loop_, getLoop(), m_pInvoker, client_uuid, req_uuid, httpRequest, params, pFnOnMessage, m_nDefRecvBuf, m_nMaxRecvBuf, m_nMaxSendQue));
	Add_HttpClient(client_uuid, clientPtr);

	return 0;
}

// url body 参数 回调函数
int HttpClientManager::Request_Asyn(uint64_t req_uuid, const boost::any& params, const HttpRequest& httpRequest)
{
	int nRet = 0;
	MutexLockGuard lock(m_csLock);
	bool nKeepAlive = httpRequest.GetKeepAlive();
	//长连接
	if (nKeepAlive)
	{
		char tmpBuff[64] = { 0 };
		std::string strIp = "";
		int nPort = 0;
		
		nPort = httpRequest.GetRequestPort();
		httpRequest.GetRequestIp(strIp);

		snprintf(tmpBuff, sizeof(tmpBuff), "%s_%d", strIp.c_str(), nPort);
		std::string strKey(tmpBuff);
		
		nRet = Request_LongConn(req_uuid, httpRequest, params, strKey);
	}
	//短链接
	else
	{
		nRet = Request_ShortConn(req_uuid, httpRequest, params);
	}
	return nRet;
}



HttpClient::HttpClient(HttpClientManager *phttpMgr, 
	EventLoop* loopBase, 
	EventLoop* loopRun, 
	void* pInvoker, 
	uint64_t client_uuid, 
	uint64_t req_uuid, 
	const HttpRequest& httpRequest, 
	const boost::any& params, 
	pfunc_on_msg_httpclient pFn,
	size_t nDefRecvBuf,
	size_t nMaxRecvBuf,
	size_t nMaxSendQue)
{
	m_llUuid = client_uuid;
	m_reqUuid = req_uuid;
	m_httpRequest = httpRequest;
	m_Params = params;
	m_pfnOnMessage = pFn;
	m_pInvoker = pInvoker;
	m_loopBase = loopBase;
	m_loopRun = loopRun;
	m_bFlage = false;
	m_phttpMgr = phttpMgr;
	m_bConnected = false;
	m_tBengin = m_tEnd = time(NULL);
	m_nDefRecvBuf = nDefRecvBuf;
	m_nMaxRecvBuf = nMaxRecvBuf;
	m_nMaxSendQue = nMaxSendQue;

	m_bIsKeepAlive = httpRequest.GetKeepAlive();
	m_pHttpResponse.reset(new HttpResponse());

	m_httpRequest.GetRequestIp(m_strIp);
	m_nPort = m_httpRequest.GetRequestPort();
	InetAddress addr(m_strIp.c_str(), static_cast<uint16_t>(m_nPort));

	char tmpBuff[64] = { 0 };
	snprintf(tmpBuff, sizeof(tmpBuff), "%s_%d", m_strIp.c_str(), m_nPort);
	m_strKey=tmpBuff;

	client_.reset(new TcpClient(m_loopRun, addr, "TcpClient", m_nDefRecvBuf, m_nMaxRecvBuf, m_nMaxSendQue));

	if (client_)
	{
		client_->setConnectionCallback(std::bind(&HttpClient::onConnection, this, std::placeholders::_1));
		client_->setMessageCallback(std::bind(&HttpClient::onMessage, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
		client_->connect();
	}
}

void HttpClient::onConnection(const TcpConnectionPtr& conn)
{
	//connect 处理
	if (!conn) return;

	if (conn->connected())
	{
		conn->setExInfo(m_pHttpResponse);
		m_bFlage = true;
		m_bConnected = true;
		send(m_reqUuid, m_Params,m_httpRequest);
	}
	else if (conn->disconnected())
	{
		//printf("disconnted_\n");
		m_phttpMgr->Del_HttpClient(m_llUuid);
	}
}

void HttpClient::onMessage(const TcpConnectionPtr&conn, Buffer* buf, Timestamp tmstamp)
{
	//数据回应处理
	if (!conn) return;

	DealHttpResponse(conn, buf, tmstamp);
}

bool HttpClient::onTimeOut()
{
	bool nRet = false;
	time_t tIntv = (time(NULL) - m_tBengin);
	int nTimeOut = m_httpRequest.GetTimeOut();
	
	//printf("HttpClient::onTimeOut,a,tIntv:%d,nTimeOut:%d,m_bFlage:%d\n", static_cast<int>(tIntv), nTimeOut, m_bFlage);
	if (tIntv >= nTimeOut)
	{
		//数据收发未完成并且超时
		if (!m_bFlage && m_pfnOnMessage)
		{
			//当作超时
			HttpResponse httpResponse;
			m_pfnOnMessage(m_pInvoker, m_llUuid, m_Params, httpResponse);
			nRet = true;
		}
		//长连接正常掉线
		if (m_bFlage && !m_bConnected && m_bIsKeepAlive)
		{
			nRet = true;
		}
		
		if (nRet)
		{
			QuitConnect();
		}
	}


	return nRet;
}

//收发完成，连接正常
bool HttpClient::isConnected()
{
	bool bRet = false;

	if (client_)
	{
		TcpConnectionPtr conn = client_->connection();
		if (conn && m_bFlage)
		{
			bRet = true;
		}
	}

	return bRet;
}

void HttpClient::QuitConnect()
{
	if (client_)
	{
		client_->quitconnect();
	}
}

void HttpClient::send(uint64_t req_uuid, const boost::any& params, const HttpRequest& httpRequest)
{
	if (client_)
	{
		TcpConnectionPtr conn = client_->connection();
		if (m_bFlage && conn)
		{
			m_httpRequest = httpRequest;
			m_reqUuid = req_uuid;
			m_Params = params;

			std::string strSendData;
			httpRequest.GetRequestStr(strSendData);
			//printf("HttpClient::send::%s\n", strSendData.c_str());
			m_bFlage = false;
			time(&m_tBengin);
			conn->send(strSendData.c_str(),  static_cast<int>(strSendData.length()));
		}
	}
}

void HttpClient::DealHttpResponse(const TcpConnectionPtr& conn, Buffer* buf, Timestamp tTime)
{
	if (HttpResponsePtr reqptr = boost::any_cast<HttpResponsePtr>(conn->getExInfo()))
	{
		StringPiece strRecvMsg = buf->toStringPiece();
		//printf("*********%ld-Recv begin**********\n%s\nsize:%d\nmsg:\n%s\n************end*************\n", conn->getConnuuid(), tTime.toFormattedString().c_str(), strRecvMsg.size(), strRecvMsg.data());
		bool bParseOver = false;

		int nParseRet = reqptr->ParseHttpRequest(strRecvMsg.data(), strRecvMsg.size(), bParseOver);

		if (0 == nParseRet)
		{
			if (bParseOver)
			{				
				if (m_pfnOnMessage)
				{
					int nRet = m_pfnOnMessage(m_pInvoker, m_llUuid, m_Params, *reqptr);

					if (nRet < 0)
					{
						conn->shutdown();
						conn->forceClose();
					}
					else if (0 == nRet)
					{
						// success
					}
					else
					{
						// other
					}
				}				
				buf->retrieveAll();
				// 复位解析器
				reqptr->ResetParser();
				// 控制标志复位
				reqptr->ResetSomeCtrlFlag();

				time(&m_tEnd);
				m_bFlage = true;
			}
			else
			{
				// 将buf中的指针归位
				buf->retrieveAll();

				// 复位解析器
				reqptr->ResetParser();
			}
		}
		else
		{
			//错误解析需要关闭且告诉上层
			if (m_pfnOnMessage)
			{
				m_pfnOnMessage(m_pInvoker, m_llUuid, m_Params, *reqptr);		
			}

			conn->shutdown();
			conn->forceClose();

			buf->retrieveAll();

			// 复位解析器
			reqptr->ResetParser();
			// 控制标志复位
			reqptr->ResetSomeCtrlFlag();
		}
	}
}
}
}