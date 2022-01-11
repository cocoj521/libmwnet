#include "../NetServer.h"
#include <mwnet_mt/util/MWEventLoop.h>
#include <mwnet_mt/util/MWThreadPool.h>

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

//#include <boost/uuid/uuid.hpp>
//#include <boost/uuid/uuid_io.hpp>
//#include <boost/uuid/uuid_generators.hpp>
#pragma GCC diagnostic ignored "-Wold-style-cast"
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wshadow"
#pragma GCC diagnostic ignored "-Wunused-variable"
#pragma GCC diagnostic ignored "-Wformat"


using namespace MWNET_MT::SERVER;
using namespace MWEVENTLOOP;
using namespace MWTHREADPOOL;


MWNET_MT::SERVER::NetServer* m_pNetServer1 = NULL;
MWNET_MT::SERVER::NetServer* m_pNetServer2 = NULL;

std::shared_ptr<MWTHREADPOOL::ThreadPool> m_pThreadPool;

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
	pfunc_on_senderr pOnSendErr; 	   			/*发送数据失败的事件回调*/
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
		bReusePort		= true;
		pOnConnection	= NULL;
		pOnReadMsg_tcp	= NULL;
		pOnReadMsg_http	= NULL;
		pOnSendOk		= NULL;
		nDefRecvBuf		= 4  * 1024;
		nMaxRecvBuf		= 32 * 1024;
		nMaxSendQue		= 64;
	}
};

template<typename T>
class AtomicIntegerT
{
 public:
  AtomicIntegerT()
    : value_(0)
  {
  }

  T get()
  {
    return __sync_val_compare_and_swap(&value_, 0, 0);
  }

  T getAndAdd(T x)
  {
    return __sync_fetch_and_add(&value_, x);
  }

  T addAndGet(T x)
  {
    return getAndAdd(x) + x;
  }

  T incrementAndGet()
  {
    return addAndGet(1);
  }

  T decrementAndGet()
  {
    return addAndGet(-1);
  }

  void add(T x)
  {
    getAndAdd(x);
  }

  void increment()
  {
    incrementAndGet();
  }

  void decrement()
  {
    decrementAndGet();
  }

  T getAndSet(T newValue)
  {
    return __sync_lock_test_and_set(&value_, newValue);
  }

 private:
  volatile T value_;
};

typedef AtomicIntegerT<int32_t> AtomicInt32;
typedef AtomicIntegerT<int64_t> AtomicInt64;
typedef AtomicIntegerT<unsigned short> AtomicUSHORT;


AtomicInt64 g_nMsgId;
AtomicUSHORT g_nSeqId;

unsigned int g_nGateNo = 0;

int64_t TranMsgIdCharToI64(const unsigned char *szMsgId)
{
	int64_t j = 0;
	const unsigned char* p = szMsgId;
	j += (int64_t)(*p) << 56;
	j += (int64_t)*(p + 1) << 48;
	j += (int64_t)*(p + 2) << 40;
	j += (int64_t)*(p + 3) << 32;
	j += (int64_t)*(p + 4) << 24;
	j += (int64_t)*(p + 5) << 16;
	j += (int64_t)*(p + 6) << 8;
	j += (int64_t)*(p + 7);
	return j;
}

int64_t MakeMsgId(unsigned int nGateNo)
{
	unsigned int nMonth = 0;
	unsigned int nDay = 0;
	unsigned int nHour = 0;
	unsigned int nMin = 0;
	unsigned int nSec = 0;
	unsigned int nGate = nGateNo;
	unsigned int nNo = g_nSeqId.getAndAdd(1);

	unsigned char MsgId[8] = {0};

  	struct timeval tv;
	struct tm	   tm_time;
	gettimeofday(&tv, NULL);
	localtime_r(&tv.tv_sec, &tm_time);
	nMonth = tm_time.tm_mon+1;
	nDay = tm_time.tm_mday;
	nHour = tm_time.tm_hour;
	nMin = tm_time.tm_min;
	nSec = tm_time.tm_sec;

	int64_t j = 0;
	j |= (int64_t)(nMonth & 0x0f) << 60;
	j |= (int64_t)(nDay & 0x1f) << 55;
	j |= (int64_t)(nHour & 0x1f) << 50;
	j |= (int64_t)(nMin & 0x3f) << 44;
	j |= (int64_t)(nSec & 0x3f) << 38;
	j |= (int64_t)(nGate & 0x03ffff) << 20;
	//j |= (INT64)(nGate  & 0x03fff)   << 20;14~18位用于长短信序号---2016-06-06,暂不调整,只需上层控制网关编号范围在0~16383即可
	j |= (int64_t)(nNo & 0x0fffff);

	MsgId[0] = (unsigned char)((j >> 56) & 0xff);
	MsgId[1] = (unsigned char)((j >> 48) & 0xff);
	MsgId[2] = (unsigned char)((j >> 40) & 0xff);
	MsgId[3] = (unsigned char)((j >> 32) & 0xff);
	MsgId[4] = (unsigned char)((j >> 24) & 0xff);
	MsgId[5] = (unsigned char)((j >> 16) & 0xff);
	MsgId[6] = (unsigned char)((j >> 8) & 0xff);
	MsgId[7] = (unsigned char)((j)& 0xff);

	return TranMsgIdCharToI64(MsgId);
}

std::string BinToHexstr(const std::string& strSrc)
{
	std::string strRet = "";
	int nSrcLen = strSrc.length();
	for(int i = 0; i < nSrcLen; ++i)
	{
		char szTmp[10] = {0};
		sprintf(szTmp, "%02x", strSrc[i]&0xff);
		strRet += szTmp;
	}
	return strRet;
}

//十六进制字符转换成整数
int HexchartoInt(char chHex)
{
	int iDec = 0;
	switch (chHex)
	{
	case '0':
	case '1':
	case '2':
	case '3':
	case '4':
	case '5':
	case '6':
	case '7':
	case '8':
	case '9':
		iDec = chHex - '0';
		break;
	case 'A':
	case 'B':
	case 'C':
	case 'D':
	case 'E':
	case 'F':
		iDec = chHex - 'A' + 10;
		break;
	case 'a':
	case 'b':
	case 'c':
	case 'd':
	case 'e':
	case 'f':
		iDec = chHex - 'a' + 10;
		break;
	default:
		iDec = 0;
		break;
	}
	return iDec;
}

//对短信内容进行HEX解码
int	TransMsg(char *szDest, const char *szSrc, int nSrcLen, int nMaxSize)
{
	if (NULL == szSrc || NULL == szDest || 0 == nSrcLen || 0 == nMaxSize)
	{
		return 0;
	}
	int nReadLen = std::min(nSrcLen, nMaxSize);
	int nLen = 0;

	for (int i = 0, k = 0; i < nReadLen; i += 2)
	{
		szDest[k++] = HexchartoInt(szSrc[i]) * 16 + HexchartoInt(szSrc[i+1]);
	}
	nLen = nReadLen/2;

	return nLen;
}


//////////////////////////////////////////////////////回调函数//////////////////////////////////////////////////////////////
//--快速操作，不可以加锁，否则将阻塞底层处理，尤其要注意，不可以加锁，否则底层多线程将变成单线程
//与连接相关的回调(连接/关闭等)
//返回值0:代表成功 >0代表其他各种错误或反控等信息(待定) <0:底层会主动强制关闭连接
int func_on_connection_tcp(void* pInvoker, uint64_t conn_uuid, const boost::any& conn, const boost::any& sessioninfo, bool bConnected, const char* szIpPort)
{
	if (bConnected)
	{
		printf("tcpserver---%d-new link-%lu-%s\n", m_pNetServer1->GetCurrTcpConnNum(),conn_uuid, szIpPort);
		boost::any session(conn_uuid);
		m_pNetServer1->SetTcpSessionInfo(conn_uuid, conn, session);
		m_pNetServer1->SetTcpHighWaterMark(conn_uuid, conn, 1024);
	}
	else
	{	
		printf("tcpserver---%d-link gone-%lu-%s\n", m_pNetServer1->GetCurrTcpConnNum(),conn_uuid, szIpPort);
		//printf("%ld\n", boost::any_cast<uint64_t>(sessioninfo));
	}

	return 0;
}
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
	
	return 0;
}

//高水位回调
int func_on_highwatermark(void* pInvoker, const uint64_t conn_uuid, const boost::any& conn, const boost::any& sessioninfo, size_t highWaterMark)
{
	printf("link:%ld, highwatermark:%d\n", conn_uuid, highWaterMark);
	
	return 0;
}

int g_nRunAsCmppSvr = 0;
#define CMPP_CONNECT                     0x00000001 //请求连接
#define CMPP_CONNECT_RESP                0x80000001 //请求连接应答
#define CMPP_SUBMIT                      0x00000004 //提交短信
#define CMPP_SUBMIT_RESP                 0x80000004 //提交短信应答
#define CMPP_ACTIVE_TEST                 0x00000008 //激活测试
#define CMPP_ACTIVE_TEST_RESP            0x80000008 //激活测试应答
int deal_cmpp_connect(void* pInvoker, uint64_t conn_uuid, const boost::any& conn, const char* szMsg, size_t nMsgLen, unsigned int nSeqId)
{
	//组回应包................................................
	unsigned int nLen = 0;
    unsigned int *pos       = 0;
    unsigned int nCmdId     = CMPP_CONNECT_RESP;
	int nStatus = 0;
	unsigned char szRsp[128] = {0};
	
    nLen = 12;

    szRsp[nLen] = (unsigned char)nStatus; 
    ++nLen;

    nLen += 16; //ISMG认证码

    szRsp[nLen] = 0x20;
    ++nLen;

    pos     = (unsigned int*)szRsp;
    *pos    = htonl(nLen);

    pos     = (unsigned int*)(szRsp + 4);
    *pos    = htonl(nCmdId);

    pos     = (unsigned int*)(szRsp + 8);
    *pos    = htonl(nSeqId);

	boost::any params;
	m_pNetServer1->SendMsgWithTcpServer(conn_uuid, conn, params, (const char*)szRsp, nLen);
	
	return 0;
}
int deal_cmpp_active(void* pInvoker, uint64_t conn_uuid, const boost::any& conn, const char* szMsg, size_t nMsgLen, unsigned int nSeqId)
{
	//组回应包................................................
	unsigned int nLen = 0;
    unsigned int *pos       = 0;
    unsigned int nCmdId     = CMPP_ACTIVE_TEST_RESP;
	unsigned char szRsp[32] = {0};
	
    nLen = 13;

    pos     = (unsigned int*)szRsp;
    *pos    = htonl(nLen);

    pos     = (unsigned int*)(szRsp + 4);
    *pos    = htonl(nCmdId);

    pos     = (unsigned int*)(szRsp + 8);
    *pos    = htonl(nSeqId);

	boost::any params;
	m_pNetServer1->SendMsgWithTcpServer(conn_uuid, conn, params, (const char*)szRsp, nLen);
	
	return 0;
}
int deal_cmpp_mt(void* pInvoker, uint64_t conn_uuid, const boost::any& conn, const char* szMsg, size_t nMsgLen, unsigned int nSeqId)
{
	//组回应包................................................
	unsigned int nLen = 0;
    unsigned int *pos       = 0;
    unsigned int nCmdId     = CMPP_SUBMIT_RESP;
	int nStatus = 0;
	unsigned char szRsp[128] = {0};
	
    nLen = 12;

	int64_t nMsgId = g_nMsgId.incrementAndGet();

	memcpy(szRsp+nLen, &nMsgId, 8);
	nLen += 8;
	
	szRsp[nLen] = (unsigned char)nStatus;
    ++nLen;
	
    pos     = (unsigned int*)szRsp;
    *pos    = htonl(nLen);

    pos     = (unsigned int*)(szRsp + 4);
    *pos    = htonl(nCmdId);

    pos     = (unsigned int*)(szRsp + 8);
    *pos    = htonl(nSeqId);

	boost::any params;
	m_pNetServer1->SendMsgWithTcpServer(conn_uuid, conn, params, (const char*)szRsp, nLen);

	return 0;
}

//处理CMPP协议
int deal_cmpp(void* pInvoker, uint64_t conn_uuid, const boost::any& conn, const char* szMsg, int nMsgLen, int& nReadedLen)
{
	unsigned int nRemain = nMsgLen;
	const char* p = szMsg;
	while (nRemain > 3)
	{
		//取出包长度
		unsigned int nPLen = (unsigned int)(p[0]&0xff)*256*256*256 + (unsigned int)(p[1]&0xff)*256*256
							+ (unsigned int)(p[2]&0xff)*256 + (unsigned int)(p[3]&0xff);
		if (nPLen < 12)
		{
			//回应错误，并断开连接
			nReadedLen = nMsgLen;
			//m_pNetServer->SendMsgWithTcpServer(conn, "ER", 2, true);
			return -1;
		}
		else if (nPLen > nRemain)
		{
			//不够长,继续接
			break;
		}
		else
		{
			//解析命令和流水号
			unsigned int nCmdId = (unsigned int)(p[4]&0xff)*256*256*256 + (unsigned int)(p[5]&0xff)*256*256
								+ (unsigned int)(p[6]&0xff)*256 + (unsigned int)(p[7]&0xff);
			unsigned int nSeqId = (unsigned int)(p[8]&0xff)*256*256*256 + (unsigned int)(p[9]&0xff)*256*256
								+ (unsigned int)(p[10]&0xff)*256 + (unsigned int)(p[11]&0xff);
			//处理.....
			switch (nCmdId)
			{
				//login....
				case CMPP_CONNECT:
					{
						if (0 != deal_cmpp_connect(pInvoker, conn_uuid, conn, p, nPLen, nSeqId)) 
						{
							nReadedLen = nMsgLen;
							return -1;
						}
					}
				break;
				//mt....
				case CMPP_SUBMIT:
					{
						if (0 != deal_cmpp_mt(pInvoker, conn_uuid, conn, p, nPLen, nSeqId))
						{
							nReadedLen = nMsgLen;
							return -1;
						}
					}
				break;
				//active...
				case CMPP_ACTIVE_TEST:
					{
						if (0 != deal_cmpp_active(pInvoker, conn_uuid, conn, p, nPLen, nSeqId))
						{
							nReadedLen = nMsgLen;
							return -1;
						}
					}
				break;
				default:
					{
						nReadedLen = nMsgLen;
						return -1;
					}
					break;
			}
			
			p			+= nPLen;
			nRemain 	-= nPLen;
			nReadedLen 	+= nPLen;
		}
	}

	return 0;
}

void send_tcp_rsp(uint64_t conn_uuid, const boost::any& conn, const boost::any& sessioninfo, const std::string& msg)
{
	//printf("send_tcp_rsp-%s\n", msg.c_str());
	boost::any params;
	int nRet = m_pNetServer1->SendMsgWithTcpServer(conn_uuid, conn, params, msg.c_str(), msg.size());
	if (0 != nRet)
	{
		printf("send_tcp_rsp-%d\n", nRet);
	}
}

//从网络层读取到信息的回调
//返回值0:代表成功 >0代表其他各种错误或反控等信息(待定) <0:底层会主动强制关闭连接
int func_on_readmsg_tcp(void* pInvoker, uint64_t conn_uuid, const boost::any& conn, const boost::any& sessioninfo, const char* szMsg, int nMsgLen, int& nReadedLen)
{
	//这里可以添加到对应的线程处理队列中去处理,尽量不要在这里处理，以免阻塞底层处理
	//添加到线程中异步处理时，必须把conn保存到队列中，调用SendMsgWithTcpServer时必须填写正确的conn，它是该连接的惟一标识
	//残包粘包需自行处理。。。
	if (1 == g_nRunAsCmppSvr)
	{
		return deal_cmpp(pInvoker, conn_uuid, conn, szMsg, nMsgLen, nReadedLen);
	}
	else 
	{
		//printf("func_on_readmsg_tcp-%s\n", szMsg);
		/*
		boost::any params;
		nReadedLen = nMsgLen;
		return m_pNetServer1->SendMsgWithTcpServer(conn_uuid, conn, params, szMsg, nMsgLen);
		*/
		///*
		nReadedLen = nMsgLen;
		std::string msg(szMsg, nMsgLen);
		if (m_pThreadPool->TryRunTask(std::bind(send_tcp_rsp, conn_uuid, conn, sessioninfo, msg)))
		{
			return 0;
		}
		else 
		{
			return -1;
		}
		//*/
	}
}


//--------------------------------------------------------
//WBS V4
int g_nRunWbsV4Jd = 0;
const char *g_szSoapEnvRsp = "<?xml version=\"1.0\" encoding=\"utf-8\"?>\r\n"
							 "<soap:Envelope xmlns:soap=\"http://schemas.xmlsoap.org/soap/envelope/\" xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" xmlns:xsd=\"http://www.w3.org/2001/XMLSchema\">\r\n"
							 "<soap:Body>\r\n"
							 "<MongateCsSpSendSmsNewResponse xmlns=\"http://tempuri.org/\">\r\n" 
							 "<MongateCsSpSendSmsNewResult>%ld</MongateCsSpSendSmsNewResult>\r\n" 
							 "</MongateCsSpSendSmsNewResponse>\r\n"
							 "</soap:Body>\r\n"
                             "</soap:Envelope>";
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
int deal_wbs_v4_jd(void* pInvoker, uint64_t conn_uuid, const boost::any& conn, const HttpRequest* pHttpReq)
{
	std::string strUrl = "";
	pHttpReq->GetUrl(strUrl);
	bool bKeepAlive = pHttpReq->IsKeepAlive();	
	HttpResponse rsp;
	if (strUrl == "/MWGate/wmgw.asmx")
	{
		rsp.SetKeepAlive(bKeepAlive);
		rsp.SetStatusCode(200);
		rsp.SetContentType("text/xml; charset=utf-8");
		std::string strBody = "";
		stdstring_format(strBody, g_szSoapEnvRsp, MakeMsgId(g_nGateNo));
		rsp.SetResponseBody(strBody);
	}
	else
	{
		rsp.SetStatusCode(400);
	}
	boost::any params;
	m_pNetServer2->SendHttpResponse(conn_uuid, conn, params, rsp);

	return 0;
}
int deal_wbs_mms(void* pInvoker, uint64_t conn_uuid, const boost::any& conn, const HttpRequest* pHttpReq)
{
	std::string strUrl = "";
	pHttpReq->GetUrl(strUrl);
	bool bKeepAlive = pHttpReq->IsKeepAlive();	

	char szTime[32] = {0};
	struct timeval tv;
	struct tm	   tm_time;
	gettimeofday(&tv, NULL);
	localtime_r(&tv.tv_sec, &tm_time);

	sprintf(szTime, "%04d%02d%02d%02d%02d%02d", 
		tm_time.tm_year + 1900, tm_time.tm_mon+1, tm_time.tm_mday, tm_time.tm_hour, tm_time.tm_min, tm_time.tm_sec);

	{
		//记录日志 时间+IP+URL
		std::string strLog = "";
		stdstring_format(strLog, "%04d-%02d-%02d %02d:%02d:%02d--%s:%u--%s\r\n", 
			tm_time.tm_year + 1900, tm_time.tm_mon+1, tm_time.tm_mday, tm_time.tm_hour, tm_time.tm_min, tm_time.tm_sec, 
			pHttpReq->GetClientRequestIp().c_str(),
			pHttpReq->GetClientRequestPort(),
			strUrl.c_str());
		std::string strLogName = "";
		stdstring_format(strLogName, "./%04d%02d%02d.txt", tm_time.tm_year + 1900, tm_time.tm_mon+1, tm_time.tm_mday);
		FILE* fp = fopen(strLogName.c_str(), "a+");
		if (fp)
		{
			fwrite(strLog.c_str(), 1, strLog.length(), fp);
			fclose(fp);
		}
	}
	
	HttpResponse rsp;

	struct stat statbuf;  
	stat("./test.mms",&statbuf);  
	int nFileSize = statbuf.st_size; 
	if (nFileSize > 0)
	{
		std::string strHexBuf = "";
		strHexBuf.resize(nFileSize);

		std::string strBody = "";
		strBody.resize(nFileSize);

		FILE* fp = fopen("./test.mms", "rb");
		if (fp)
		{
			int nReadLen = fread((char*)strHexBuf.data(), 1, nFileSize, fp);

			char szTime[32] = {0};
			struct timeval tv;
			struct tm	   tm_time;
			gettimeofday(&tv, NULL);
			localtime_r(&tv.tv_sec, &tm_time);
	
			sprintf(szTime, "%04d%02d%02d%02d%02d%02d", 
				tm_time.tm_year + 1900, tm_time.tm_mon+1, tm_time.tm_mday, tm_time.tm_hour, tm_time.tm_min, tm_time.tm_sec);
			//组头
			std::string strMMSHeader = "";
			//transactionid
			strMMSHeader = "8c84";
			strMMSHeader += "98";
			strMMSHeader += BinToHexstr(szTime);
			strMMSHeader += "00";
			
			//version
			strMMSHeader += "8d90";
			
			//messageid
			strMMSHeader += "8b";
			strMMSHeader += BinToHexstr(szTime);
			strMMSHeader += "00";
			
			//DATE
			strMMSHeader += "8504";
			time_t tt = time(NULL)-3;
			char szDate[32] = {0};
			sprintf(szDate, "%08x", tt);
			strMMSHeader += szDate;

			memcpy((void*)strHexBuf.data(), strMMSHeader.c_str(), strMMSHeader.length());

			int nBodyLen = TransMsg((char*)strBody.data(), strHexBuf.c_str(), nFileSize, nFileSize);
			strBody.resize(nBodyLen);
			
			fclose(fp);
			
			rsp.SetKeepAlive(bKeepAlive);
			rsp.SetStatusCode(200);
			rsp.SetContentType("application/vnd.wap.mms-message");
			rsp.SetResponseBody(strBody);
		}
		else
		{
			rsp.SetStatusCode(403);
		}

	}
	else
	{
		rsp.SetStatusCode(403);
	}

	boost::any params;
	m_pNetServer2->SendHttpResponse(conn_uuid, conn, params, rsp);

	return 0;
}

//从网络层读取到信息的回调
//返回值0:代表成功 >0代表其他各种错误或反控等信息(待定) <0:底层会主动强制关闭连接
//expect:100-continue底层会自动处理并自动回复，不会再往上层回调，上层不需要处理
int func_on_readmsg_http(void* pInvoker, uint64_t conn_uuid, const boost::any& conn, const boost::any& sessioninfo, const HttpRequest* pHttpReq)
{
	if (1 == g_nRunWbsV4Jd) 
	{
		return deal_wbs_v4_jd(pInvoker, conn_uuid, conn, pHttpReq);
	}
	else if (2 == g_nRunWbsV4Jd)
	{
		return deal_wbs_mms(pInvoker, conn_uuid, conn, pHttpReq);
	}
	std::string strUrl = "";std::string strBody = "";
	pHttpReq->GetUrl(strUrl);
	//printf("func_on_readmsg_http:%s\n", strUrl.c_str());
	bool bKeepAlive = pHttpReq->IsKeepAlive();
	//printf("keepalive:%d\n", bKeepAlive);
	//pHttpReq->GetBodyData(strBody);
	//printf("bodydata:%s\n", strBody.c_str());
	
	HttpResponse rsp;
	//这里根据url的不同进行不同的处理，并生成不同的回应(geturl函数的返回值是区分大小的，若不想区分，自行处理)
	//可以添加到对应的线程处理队列中去处理,尽量不要在这里处理，以免阻塞底层处理
	//添加到线程中异步处理前，必须把httprequest中的信息提前保存，不可以直接保存它，因为它在回调函数结束后，会自动析构
	//添加到线程中异步处理时，必须把conn保存到队列中，调用SendHttpResponse时必须填写正确的conn，它是该连接的惟一标识
	//200 OK的包必须设置正确的contenttype,responsebody
	//非200 OK的包不需要设置ResponseBody，没有必要就不用设置responsebody，只需要设置StatusCode，底层会根据StatusCode自动回应相应的报文，如果想详细描述错误原因，可以设responsebody
	//keepalive不是必设项，若不显式设定的话，会全部按照Connection: Close处理
	if (strUrl == "/sms/v2/std/single_send")
	{
		std::string strXUpBearType = "";
		//pHttpReq->GetXUpBearType(strXUpBearType);
		//strXUpBearType = "";
		pHttpReq->GetHttpHeaderFieldValue("testaaa", strXUpBearType);
		rsp.SetKeepAlive(bKeepAlive);
		rsp.SetStatusCode(200);
		rsp.SetContentType("text/html");
		rsp.SetResponseBody("<html><head><title>test</title></head><body>Hello WORLD!</body></html>");
	}
	else if (strUrl == "/sms/v2/std/batch_send")
	{
		rsp.SetKeepAlive(bKeepAlive);
		rsp.SetStatusCode(200);
		rsp.SetContentType("application/x-www-form-urlencoded; charset=utf-8");
		rsp.SetResponseBody("cmd=rsp&ret=0");
	}
	else
	{
		printf("######****************bad request:%s\n", strUrl.c_str());
		rsp.SetStatusCode(400);
		//rsp.SetResponseBody("自定义。。。");
	}

	boost::any params;
	m_pNetServer2->SendHttpResponse(conn_uuid, conn, params, rsp);

	return 0;
}
//发送数据完成的回调
//返回值0:代表成功 >0代表其他各种错误或反控等信息(待定) <0:底层会主动强制关闭连接
int func_on_sendok_tcp(void* pInvoker, const uint64_t conn_uuid, const boost::any& conn, const boost::any& sessioninfo, const boost::any& params)
{
	//printf("pfunc_on_sendok_tcp...%ld\n", conn_uuid);
	return 0;
}
int func_on_senderr_tcp(void* pInvoker, const uint64_t conn_uuid, const boost::any& conn, const boost::any& sessioninfo, const boost::any& params, int errCode)
{
	printf("pfunc_on_senderr_tcp...%d\n", errCode);
	return 0;
}
//发送数据完成的回调
//返回值0:代表成功 >0代表其他各种错误或反控等信息(待定) <0:底层会主动强制关闭连接
int func_on_sendok_http(void* pInvoker, const uint64_t conn_uuid, const boost::any& conn, const boost::any& sessioninfo, const boost::any& params)
{
	//printf("pfunc_on_sendok_http...%ld\n", conn_uuid);
	return 0;
}
int func_on_senderr_http(void* pInvoker, const uint64_t conn_uuid, const boost::any& conn, const boost::any& sessioninfo, const boost::any& params, int errCode)
{
	printf("pfunc_on_senderr_http...%d\n", errCode);
	return 0;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void* StartTcpServer(void* p)
{
	::prctl(PR_SET_NAME,"RunTcpServer");
	NETSERVER_CONFIG* pConfig = reinterpret_cast<NETSERVER_CONFIG*>(p);
	if (pConfig)
	{
	AGAIN:
		m_pNetServer1->EnableTcpServerRecvLog();
		m_pNetServer1->EnableTcpServerSendLog();
		printf("tcpserver:%u start success...\n",pConfig->listenport);
		m_pNetServer1->StartTcpServer(pConfig->pInvoker, pConfig->listenport, 
			pConfig->threadnum, pConfig->idleconntime, 
			pConfig->maxconnnum, pConfig->bReusePort, 
			pConfig->pOnConnection, pConfig->pOnReadMsg_tcp, pConfig->pOnSendOk, 
			pConfig->pOnSendErr, pConfig->pOnHighWaterMark,
			pConfig->nDefRecvBuf, pConfig->nMaxRecvBuf, pConfig->nMaxSendQue);	
	goto AGAIN;
	}
	else
	{
		printf("tcpserver start failed...\n");
	}
	printf("tcpserver quit...\n");
	return NULL;
}

void* StartTcpServer2(void* p)
{
	::prctl(PR_SET_NAME,"RunTcpServer2");
	NETSERVER_CONFIG* pConfig = reinterpret_cast<NETSERVER_CONFIG*>(p);
	if (pConfig)
	{
		m_pNetServer2->EnableTcpServerRecvLog();
		m_pNetServer2->EnableTcpServerSendLog();
		printf("tcpserver:%u start success...\n",pConfig->listenport);
		m_pNetServer2->StartTcpServer(pConfig->pInvoker, pConfig->listenport, 
			pConfig->threadnum, pConfig->idleconntime, 
			pConfig->maxconnnum, pConfig->bReusePort, 
			pConfig->pOnConnection, pConfig->pOnReadMsg_tcp, pConfig->pOnSendOk, 
			pConfig->pOnSendErr, pConfig->pOnHighWaterMark,
			pConfig->nDefRecvBuf, pConfig->nMaxRecvBuf, pConfig->nMaxSendQue);	
	}
	else
	{
		printf("tcpserver start failed...\n");
	}
	printf("tcpserver quit...\n");
	return NULL;
}

void* StartHttpServer(void* p)
{
	::prctl(PR_SET_NAME,"RunHttpServer");
	NETSERVER_CONFIG* pConfig = reinterpret_cast<NETSERVER_CONFIG*>(p);
	if (pConfig)
	{
		m_pNetServer2->EnableHttpServerRecvLog();
		m_pNetServer2->EnableHttpServerSendLog();
		printf("httpserver:%u start success...\n",pConfig->listenport);
		m_pNetServer2->StartHttpServer(pConfig->pInvoker, pConfig->listenport, 
			pConfig->threadnum, pConfig->idleconntime, 
			pConfig->maxconnnum, pConfig->bReusePort, 
			pConfig->pOnConnection, pConfig->pOnReadMsg_http, pConfig->pOnSendOk, 
			pConfig->pOnSendErr, pConfig->pOnHighWaterMark,
			pConfig->nDefRecvBuf, pConfig->nMaxRecvBuf, pConfig->nMaxSendQue);	
	}
	else
	{
		printf("httpserver start failed...\n");
	}
	printf("httpserver quit...\n");
	return NULL;
}

class AAAA
{
	public:
	AAAA(){};
	~AAAA(){printf("aaa......\n");};
};

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


//using namespace MWEVENTLOOP;

void MyPrint1()
{
	printf("myprint1....\n");
}

void MyPrint2()
{
	printf("myprint2....\n");
}

void MyPrint3()
{
	printf("myprint3....\n");
}
/*
int main()
{
	MWEVENTLOOP::EventLoop evloop;

	evloop.RunOnce(MyPrint1);

	evloop.RunAfter(10.0, MyPrint2);

	evloop.RunEvery(1.0, MyPrint3);
}
*/
int main(int argc, char* argv[])
{
	char path[256] = {0};
    char processname[256] = {0};
    get_executable_path(path, processname, sizeof(path));
		
	::prctl(PR_SET_NAME, processname);

/*
{
	MWEVENTLOOP::EventLoop evloop;

	evloop.RunOnce(MyPrint1);

	evloop.RunAfter(10.0, MyPrint2);

	evloop.RunEvery(1.0, MyPrint3);

	evloop.QuitLoop();
}
	while (1)
	{
		sleep(1);
	}
	*/
	/*
	time_t tBegin = time(NULL);
	for (int itest = 0; itest < 10000; ++itest)
	{
	boost::uuids::string_generator gen;
	gen("01234567-89ab-cdef-0123-456789abcdef");
	}
	int ndiff = time(NULL)-tBegin;
	printf("%d\n", ndiff);
	*/
/*
{
	boost::any any_aaapt;
{
	boost::shared_ptr<AAAA> aaaptr(new AAAA());
	any_aaapt=aaaptr;
}	
}
*/
/*	
	char any[16] = {0};
{
	boost::shared_ptr<int> int_ptr(new int);
	*int_ptr = 100;
	{
		boost::weak_ptr<int> weak_int_ptr(int_ptr);
		memcpy(any, &weak_int_ptr, sizeof(weak_int_ptr));
	}
}
	//还原
	boost::weak_ptr<int> weak_int_ptr2;
	memcpy(&weak_int_ptr2, any, sizeof(any));
	boost::shared_ptr<int> int_ptr2 = weak_int_ptr2.lock();
	if (int_ptr2)
	{
		int n = *int_ptr2;
	}

	//清空,看是否会报错
	boost::weak_ptr<int> weak_int_ptr3;
	memset(any, 0, sizeof(any));
	memcpy(&weak_int_ptr3, any, sizeof(any));
	boost::shared_ptr<int> int_ptr3 = weak_int_ptr3.lock();
	if (int_ptr3)
	{
		int n = *int_ptr3;
	}

	//改个乱七八糟的值,看是否会报错
	boost::weak_ptr<int> weak_int_ptr4;
	char szTmp[16] = "123456789012345";
	memcpy(any, szTmp, sizeof(any));
	memcpy(&weak_int_ptr4, any, sizeof(any));
	boost::shared_ptr<int> int_ptr4 = weak_int_ptr4.lock();
	if (int_ptr4)
	{
		int n = *int_ptr4;
	}
	*/
	g_nMsgId.getAndSet(0);
	NETSERVER_CONFIG* pTcpConfig = new NETSERVER_CONFIG();
	NETSERVER_CONFIG* pHttpConfig = new NETSERVER_CONFIG();
	m_pNetServer1 = new MWNET_MT::SERVER::NetServer(1);
	m_pNetServer2 = new MWNET_MT::SERVER::NetServer(2);
	m_pThreadPool.reset(new MWTHREADPOOL::ThreadPool("WORKTHRPOOL"));

	pTcpConfig->pOnHighWaterMark = func_on_highwatermark;
	pHttpConfig->pOnHighWaterMark = func_on_highwatermark;
	
	pthread_t ntid;
	int 	err = 0;	
	
	int nMode = 3;
	printf("input run mode:1:tcpserver 2:httpserver 3:both\n");
	scanf("%d", &nMode);
	
	if (1 == nMode)
	{
		printf("input tcpserver threadnum:\n");
		scanf("%d", &pTcpConfig->threadnum);
		printf("tcpserver threadnum:%d\n", pTcpConfig->threadnum);

		printf("input tcpserver port:\n");
		scanf("%hu", &pTcpConfig->listenport);
		printf("tcpserver port:%u\n", pTcpConfig->listenport);

		printf("do you want tcpserver run cmpp:1 or 0\n");
		scanf("%d", &g_nRunAsCmppSvr);

		if (g_nRunAsCmppSvr)
		{
			printf("please input gateno(must not the same as others)\n");
			scanf("%d", &g_nGateNo);
		}

		int nPoolSize = 8;
		printf("input wrok thread pool size:\n");
		scanf("%d", &nPoolSize);

		int nTaskQue = 100000;
		printf("input thread pool task que maxsize:\n");
		scanf("%d", &nTaskQue);

		m_pThreadPool->SetMaxTaskQueueSize(nTaskQue);

		m_pThreadPool->StartThreadPool(nPoolSize);

		
		//tcp回调函数
		pTcpConfig->pOnConnection	= func_on_connection_tcp;
		pTcpConfig->pOnReadMsg_tcp	= func_on_readmsg_tcp;
		pTcpConfig->pOnSendOk		= func_on_sendok_tcp;
		pTcpConfig->pOnSendErr		= func_on_senderr_tcp;
		pTcpConfig->nDefRecvBuf		= 4  * 1024;
		pTcpConfig->nMaxRecvBuf		= 32 * 1024;
		pTcpConfig->nMaxSendQue		= 64;

		//启动tcpserver
		err = pthread_create(&ntid, NULL, StartTcpServer, reinterpret_cast<void*>(pTcpConfig));
		if (err != 0) printf("StartTcp can't create thread: %s\n", strerror(err));
		pthread_detach(ntid);

		//sleep(5);

		//pTcpConfig->listenport = 9999;
		//启动tcpserver
		//err = pthread_create(&ntid, NULL, StartTcpServer2, reinterpret_cast<void*>(pTcpConfig));
		//if (err != 0) printf("StartTcp can't create thread: %s\n", strerror(err));
		//pthread_detach(ntid);
	}
	else if (2 == nMode)
	{
		printf("input httpserver threadnum:\n");
		scanf("%d", &pHttpConfig->threadnum);
		printf("httpserver threadnum:%d\n", pHttpConfig->threadnum);

		printf("input httpserver port:\n");
		scanf("%hu", &pHttpConfig->listenport);
		printf("httpserver port:%u\n", pHttpConfig->listenport);

		printf("select wbs model: 0-skip 1-wbsv4 2-mmssvr\n");
		scanf("%d", &g_nRunWbsV4Jd);

		if (g_nRunWbsV4Jd)
		{
			printf("please input gateno(must not the same as others)\n");
			scanf("%d", &g_nGateNo);
		}

		int nPoolSize = 8;
		printf("input wrok thread pool size:\n");
		scanf("%d", &nPoolSize);

		int nTaskQue = 100000;
		printf("input thread pool task que maxsize:\n");
		scanf("%d", &nTaskQue);

		m_pThreadPool->SetMaxTaskQueueSize(nTaskQue);

		m_pThreadPool->StartThreadPool(nPoolSize);
		
		//http回调函数
		pHttpConfig->pOnConnection	= func_on_connection_http;
		pHttpConfig->pOnReadMsg_http= func_on_readmsg_http;
		pHttpConfig->pOnSendOk		= func_on_sendok_http;
		pHttpConfig->pOnSendErr		= func_on_senderr_http;
		pHttpConfig->nDefRecvBuf	= 4  * 1024;
		pHttpConfig->nMaxRecvBuf	= 32 * 1024;
		pHttpConfig->nMaxSendQue	= 64;
		
		//启动httpserver
		err = pthread_create(&ntid, NULL, StartHttpServer, reinterpret_cast<void*>(pHttpConfig));
		if (err != 0) printf("StartHttp can't create thread: %s\n", strerror(err));
		pthread_detach(ntid);
	}
	else if (3 == nMode)
	{
		printf("input tcpserver threadnum:\n");
		scanf("%d", &pTcpConfig->threadnum);
		printf("tcpserver threadnum:%d\n", pTcpConfig->threadnum);
		
		printf("input httpserver threadnum:\n");
		scanf("%d", &pHttpConfig->threadnum);
		printf("httpserver threadnum:%d\n", pHttpConfig->threadnum);

		printf("input tcpserver port:\n");
		scanf("%hu", &pTcpConfig->listenport);
		printf("tcpserver port:%u\n", pTcpConfig->listenport);

		printf("input httpserver port:\n");
		scanf("%hu", &pHttpConfig->listenport);
		printf("httpserver port:%u\n", pHttpConfig->listenport);

		printf("do you want tcpserver run cmpp:1 or 0\n");
		scanf("%d", &g_nRunAsCmppSvr);

		printf("do you want httpserver run wbsv4:1 or 0\n");
		scanf("%d", &g_nRunWbsV4Jd);

		if (g_nRunAsCmppSvr || g_nRunWbsV4Jd)
		{
			printf("please input gateno(must not the same as others)\n");
			scanf("%d", &g_nGateNo);
		}

		int nPoolSize = 8;
		printf("input wrok thread pool size:\n");
		scanf("%d", &nPoolSize);

		int nTaskQue = 100000;
		printf("input thread pool task que maxsize:\n");
		scanf("%d", &nTaskQue);

		m_pThreadPool->SetMaxTaskQueueSize(nTaskQue);

		m_pThreadPool->StartThreadPool(nPoolSize);
		
		//tcp回调函数
		pTcpConfig->pOnConnection	= func_on_connection_tcp;
		pTcpConfig->pOnReadMsg_tcp	= func_on_readmsg_tcp;
		pTcpConfig->pOnSendOk		= func_on_sendok_tcp;
		pTcpConfig->pOnSendErr      = func_on_senderr_tcp;
		
		pTcpConfig->nDefRecvBuf		= 4  * 1024;
		pTcpConfig->nMaxRecvBuf		= 32 * 1024;
		pTcpConfig->nMaxSendQue		= 64;


		//http回调函数
		pHttpConfig->pOnConnection	= func_on_connection_http;
		pHttpConfig->pOnReadMsg_http= func_on_readmsg_http;
		pHttpConfig->pOnSendOk		= func_on_sendok_http;
		pHttpConfig->pOnSendErr     = func_on_senderr_http;
		
		pHttpConfig->nDefRecvBuf	= 4  * 1024;
		pHttpConfig->nMaxRecvBuf	= 32 * 1024;
		pHttpConfig->nMaxSendQue	= 64;

		//启动tcpserver
		err = pthread_create(&ntid, NULL, StartTcpServer, reinterpret_cast<void*>(pTcpConfig));
		if (err != 0) printf("StartTcp can't create thread: %s\n", strerror(err));
		pthread_detach(ntid);

		//启动httpserver
		err = pthread_create(&ntid, NULL, StartHttpServer, reinterpret_cast<void*>(pHttpConfig));
		if (err != 0) printf("StartHttp can't create thread: %s\n", strerror(err));
		pthread_detach(ntid);
	}
	//sleep(5);
	//m_pNetServer->StopTcpServer();
	//m_pNetServer2->StopTcpServer();

	//sleep(5);
	//return 0;
	while(1)
	{
		static int64_t last_tcp_cnt = 0;
		static int64_t last_http_cnt = 0;
		static time_t tStart = time(NULL);
		sleep(1);
		printf("tcp  --- totallink:%d-totalreq:%ld-spd:%ld-waitsend:%ld\nhttp --- totallink:%d-totalreq:%ld-spd:%ld-waitsend:%ld\n\n", 
			m_pNetServer1->GetCurrTcpConnNum(),
			m_pNetServer1->GetTotalTcpReqNum(),
			m_pNetServer1->GetTotalTcpReqNum()-last_tcp_cnt,
			m_pNetServer1->GetTotalTcpWaitSendNum(),
			m_pNetServer2->GetCurrHttpConnNum(),
			m_pNetServer2->GetTotalHttpReqNum(),
			m_pNetServer2->GetTotalHttpReqNum()-last_http_cnt,
			m_pNetServer2->GetTotalHttpWaitSendNum());
		last_tcp_cnt = m_pNetServer1->GetTotalTcpReqNum();
		last_http_cnt = m_pNetServer2->GetTotalHttpReqNum();

		if (time(NULL) - tStart > 10)
		{
			//m_pNetServer1->StopTcpServer();
			time(&tStart);
			//delete m_pNetServer1;
			//break;
		}
	}
}
