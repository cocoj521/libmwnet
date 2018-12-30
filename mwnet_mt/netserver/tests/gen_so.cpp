#include "../NetServer.h"

#include <stdint.h>
#include <pthread.h>
#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>

#include <boost/any.hpp>
#include <sys/prctl.h>
#include <sys/syscall.h>
#include <arpa/inet.h>
#include <stdarg.h>
#include <algorithm>
#include <sys/time.h>
#include <sys/stat.h>
#include <unistd.h>
#include <cmath>

#pragma GCC diagnostic ignored "-Wold-style-cast"
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wshadow"
#pragma GCC diagnostic ignored "-Wunused-variable"
#pragma GCC diagnostic ignored "-Wformat"


using namespace MWNET_MT::SERVER;


MWNET_MT::SERVER::NetServer* m_pHttpServer = NULL;

//网络服务器配置参数
struct NETSERVER_CONFIG
{
	void* pInvoker; 						/*上层调用的类指针*/
	uint16_t listenport;					/*监听端口*/
	int threadnum;							/*线程数*/
	int idleconntime;						/*空连接踢空时间*/
	int maxconnnum; 						/*最大连接数*/
	bool bReusePort;						/*是否启用SO_REUSEPORT*/
	pfunc_on_connection pOnConnection; 		/*与连接相关的事件回调(connect/close/error etc.)*/
	pfunc_on_readmsg_tcp pOnReadMsg_tcp;	/*网络层收到数据的事件回调-用于tcpserver*/
	pfunc_on_readmsg_http pOnReadMsg_http;	/*网络层收到数据的事件回调-用于httpserver*/
	pfunc_on_sendok pOnSendOk; 	   			/*发送数据完成的事件回调*/	
	pfunc_on_highwatermark pOnHighWaterMark;/*高水位事件回调*/
	size_t nDefRecvBuf;						/*默认接收缓冲,与nMaxRecvBuf一样根据真实业务数据包大小进行设置,该值设置为大部分业务包可能的大小*/
	size_t nMaxRecvBuf;						/*最大接收缓冲,若超过最大接收缓冲仍然无法组成一个完整的业务数据包,连接将被强制关闭,取值时务必结合业务数据包大小进行填写*/
	size_t nMaxSendQue; 					/*最大发送缓冲队列,网络层最多可以容忍的未发送出去的包的总数,若超过该值,将会返回高水位回调,上层必须根据pfunc_on_sendok控制好滑动*/

	NETSERVER_CONFIG()
	{
		pInvoker 		= NULL;
		listenport 		= 5958;
		threadnum 		= 16;
		idleconntime	= 60*15;
		maxconnnum		= 1000000;
		bReusePort		= false;
		pOnConnection	= NULL;
		pOnReadMsg_tcp	= NULL;
		pOnReadMsg_http	= NULL;
		pOnSendOk		= NULL;
		nDefRecvBuf		= 4  * 1024;
		nMaxRecvBuf		= 32 * 1024;
		nMaxSendQue		= 64;
	}
};

void stdstring_format(std::string& strRet, const char *fmt, ...)
{
	va_list marker;
	va_start(marker, fmt);
	int nLength = vsnprintf(NULL, 0, fmt, marker);
	va_end(marker);
	if (nLength < 0) return;
	va_start(marker, fmt);
	strRet.resize(nLength + 1);
	int nSize = vsnprintf((char*)strRet.data(), nLength + 1, fmt, marker);
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


//////////////////////////////////////////////////////回调函数//////////////////////////////////////////////////////////////

//与连接相关的回调(连接/关闭等)
//返回值0:代表成功 >0代表其他各种错误或反控等信息(待定) <0:底层会主动强制关闭连接
int func_on_connection_http(void* pInvoker, uint64_t conn_uuid, const boost::any& conn, const boost::any& sessioninfo, bool bConnected, const char* szIpPort)
{
	if (bConnected)
	{
		//printf("httpserver---%d-new link %ld-%s\n", m_pNetServer->GetCurrHttpConnNum(),sockid,szIpPort);
	}
	else
	{	
		//printf("httpserver---%d-link gone %ld-%s\n", m_pNetServer->GetCurrHttpConnNum(),sockid,szIpPort);
	}
	// 不用处理
	return 0;
}

//高水位回调
int func_on_highwatermark(void* pInvoker, const uint64_t conn_uuid, const boost::any& conn, const boost::any& sessioninfo, size_t highWaterMark)
{
	printf("link:%ld, highwatermark:%d\n", conn_uuid, highWaterMark);
	// 不用处理
	return 0;
}

//从网络层读取到信息的回调
//返回值0:代表成功 >0代表其他各种错误或反控等信息(待定) <0:底层会主动强制关闭连接
//expect:100-continue底层会自动处理并自动回复，不会再往上层回调，上层不需要处理
int func_on_readmsg_http(void* pInvoker, uint64_t conn_uuid, const boost::any& conn, const boost::any& sessioninfo, const HttpRequest* pHttpReq)
{
	std::string strUrl = "";
	std::string strBody = "";
	pHttpReq->GetUrl(strUrl);	
	
	HttpResponse rsp;
	
	if (strUrl == "/push_report")
	{
		std::string strBody = "";

		// 取出BODY
		pHttpReq->GetBodyData(strBody);
		
		printf("###*****body:%s\n", strBody.c_str());
		// 解析JSON

		// 加入状态报告队列
		// ..............
		
		// 回应
		rsp.SetKeepAlive(false);
		rsp.SetStatusCode(200);
		rsp.SetContentType("application/json");
		rsp.SetResponseBody("{\"ResCode\":1000,\"ResMsg\":\"成功\"}");
	}
	else
	{
		printf("######****************bad request:%s\n", strUrl.c_str());
		rsp.SetStatusCode(400);
	}

	boost::any params;
	m_pHttpServer->SendHttpResponse(conn_uuid, conn, params, rsp);

	return 0;
}

//发送数据完成的回调
//返回值0:代表成功 >0代表其他各种错误或反控等信息(待定) <0:底层会主动强制关闭连接
int func_on_sendok_http(void* pInvoker, const uint64_t conn_uuid, const boost::any& conn, const boost::any& sessioninfo, const boost::any& params)
{
	//printf("pfunc_on_sendok_http...%ld\n", sockid);
	// 不用处理
	return 0;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void* StartHttpServer(void* p)
{
	::prctl(PR_SET_NAME,"RunHttpServer");
	NETSERVER_CONFIG* pConfig = reinterpret_cast<NETSERVER_CONFIG*>(p);
	if (pConfig)
	{
		m_pHttpServer->EnableHttpServerRecvLog();
		m_pHttpServer->EnableHttpServerSendLog();
		printf("httpserver:%u start success...\n",pConfig->listenport);
		m_pHttpServer->StartHttpServer(pConfig->pInvoker, pConfig->listenport, 
			pConfig->threadnum, pConfig->idleconntime, 
			pConfig->maxconnnum, pConfig->bReusePort, 
			pConfig->pOnConnection, pConfig->pOnReadMsg_http, pConfig->pOnSendOk, pConfig->pOnHighWaterMark,
			pConfig->nDefRecvBuf, pConfig->nMaxRecvBuf, pConfig->nMaxSendQue);	
	}
	else
	{
		printf("httpserver start failed...\n");
	}
	printf("httpserver quit...\n");
	return NULL;
}


size_t get_executable_path(char* processdir, char* processname, size_t len)
{
    char* path_end = NULL;
    if(::readlink("/proc/self/exe", processdir,len) <=0) return -1;
    path_end = strrchr(processdir,  '/');
    if(path_end == NULL)  return -1;
    ++path_end;
    strcpy(processname, path_end);
    *path_end = '\0';
    return (size_t)(path_end - processdir);
}

int test_so()
{
/*
	char path[256] = {0};
    char processname[256] = {0};
    get_executable_path(path, processname, sizeof(path));
		
	::prctl(PR_SET_NAME, processname);
*/
// 可以把下面这一段拷到init函数中执行，必须启一个线程！
{
	NETSERVER_CONFIG* pHttpConfig = new NETSERVER_CONFIG();
	// 监听端口
	pHttpConfig->listenport = 8099;
	// 工作线程
	pHttpConfig->threadnum  = 4;
	// 踢空
	pHttpConfig->idleconntime = 30;
	//http回调函数
	pHttpConfig->pOnHighWaterMark = func_on_highwatermark;
	pHttpConfig->pOnConnection	= func_on_connection_http;
	pHttpConfig->pOnReadMsg_http= func_on_readmsg_http;
	pHttpConfig->pOnSendOk		= func_on_sendok_http;
	pHttpConfig->nDefRecvBuf	= 4 * 1024;
	pHttpConfig->nMaxRecvBuf	= 8 * 1024;
	pHttpConfig->nMaxSendQue	= 512;
	
	m_pHttpServer = new MWNET_MT::SERVER::NetServer(1);

	//启动httpserver	
	pthread_t ntid;	
	int err = pthread_create(&ntid, NULL, StartHttpServer, reinterpret_cast<void*>(pHttpConfig));
	if (err != 0) printf("StartHttp can't create thread: %s\n", strerror(err));
	pthread_detach(ntid);
}
	/*
	while(1)
	{
		sleep(1);
	}
	*/
}
