#include "../CurlHttpClient.h"
#include <functional>
#include <mwnet_mt/base/LogFile.h>
#include <mwnet_mt/base/Logging.h>
#include <mwnet_mt/base/Logging.h>
#include <mwnet_mt/base/ThreadPool.h>
#include <string>
#include <sys/stat.h>

std::string strBody = 
"<?xml version=\"1.0\" encoding=\"utf-8\"?>"
"<soap:Envelope>"
"<soap:Body>"
"<MongateCsSpSendSmsNew xmlns=\"http://tempuri.org/\">"
"<userId>TSTSTS</userId>"
"<password>456123</password>"
"<pszMobis>13265661403</pszMobis>"
"<pszMsg>test,test,test,test,test,test,test,test</pszMsg>"
"<iMobiCount>1</iMobiCount>"
"<pszSubPort>*</pszSubPort>"
"</MongateCsSpSendSmsNew>"
"</soap:Body>"
"</soap:Envelope>";
const std::string https_url = "https://tscs1.800ct.cn:7102/MWGate/wmgw.asmx";
//const std::string https_url = "https://61.145.229.28:7102/MWGate/wmgw.asmx";

const std::string https_jdtest = "https://175.25.17.163:8022/MWGate/wmgw.asmx/MongateCsSpSendSmsNew";
const std::string http_jdtest = "http://175.25.17.163:8026/MWGate/wmgw.asmx/MongateCsSpSendSmsNew";

//const std::string https_url = "https://www.baidu.com";
//const std::string url2 = "https://192.169.2.26:8088/MWGate/wmgw.asmx";
//const std::string url1 = "http://192.169.2.53:8088/MWGate/wmgw.asmx";
//const std::string url2 = "http://192.169.2.53:8088/MWGate/wmgw.asmx";
const std::string http_url = "http://192.169.0.231:1235/sms/v2/std/single_send";
//const std::string http_url = "http://192.169.1.130:1234/sms/v2/std/single_send";
//const std::string http_url = "http://TSCS1.800CT.CN:7902/MWGate/wmgw.asmx/MongateCsSpSendSmsNew";
//const std::string http_url = "http://61.145.229.28:7902/MWGate/wmgw.asmx/MongateCsSpSendSmsNew";
char g_szHttpUrl[2048+1] = {0};
char g_szHttpBody[4096+1] = {0};
int g_nKeepAlive = 0;
typedef std::function<void ()> THREAD_TASK;

using namespace MWNET_MT;
using namespace MWNET_MT::CURL_HTTPCLIENT;

// 回调函数
int func_onmsg_cb(void* pInvoker, uint64_t req_uuid, const boost::any& params, const HttpResponsePtr& response)
{
	if (response->GetErrCode() != 0)
	{
		LOG_INFO 
				 << "URL:" << response->GetEffectiveUrl() << ",code:" << response->GetErrCode() << ",desc:" << response->GetErrDesc() 
			     << "\n" << "ResponseCode:" << response->GetRspCode() 
			     << " totaltm:" << response->GetReqTotalConsuming()/1000 << "ms"
			     << " conntm:" << response->GetReqConnectConsuming()/1000 << "ms"
			     << " nameloopuptm:" << response->GetReqNameloopupConsuming()/1000 << "ms"
			     << " reqinquetm:"<<response->GetReqInQueTime()/1000 << "ms"
				 << " reqsdtm:" <<response->GetReqSendTime()/1000 << "ms"
			     << " rsprecvtm:" << response->GetRspRecvTime()/1000 << "ms"
			     << "\n" << response->GetHeader() << response->GetBody();
	}
	return 0;
}

// 全局httpclient对象
CurlHttpClient http_cli;

// 线程任务函数
void test(int post_get, const std::string& strHttpUrl, const std::string& strHttpBody)
{
	std::string strBodyTmp = strHttpBody;
	HttpRequestPtr http_request = http_cli.GetHttpRequest(strHttpUrl, 
														HTTP_1_1, 
														1==post_get?HTTP_REQUEST_POST:HTTP_REQUEST_GET, 
														g_nKeepAlive);
	if (http_request)
	{
		LOG_DEBUG << "GetHttpRequest:" << http_request->GetReqUUID();

		
		http_request->SetTimeOut(30, 60);
		//http_request->SetKeepAliveTime();
		//http_request->SetHeader("myheader1", "nothing1");
		//http_request->SetHeader("myheader2", "nothing2");
		//http_request->SetContentType("text/json");
		http_request->SetBody(strBodyTmp);
		
		boost::any params(http_request);
		while (0 != http_cli.SendHttpRequest(http_request, params, false))
		{
			//LOG_INFO << "-=-=-=-=-=-=-=SendHttpRequest Error-=-=-=-==-=";
			usleep(1000*10);
		}
		
		//sleep(5);
		//http_cli.CancelHttpRequest(http_request);
		//LOG_INFO << "-=-=-=-=-=-=-=CancelHttpRequest-=-=-=-==-=";
		//sleep(3600);
		LOG_DEBUG << "SendHttpRequest:" << http_request->GetReqUUID();
	}
	else
	{
		printf("GetHttpRequest Error\n");		
		usleep(1*1);
	}
}

typedef std::shared_ptr<mwnet_mt::ThreadPool> ThreadPoolPtr;

//日志
//..........................................................
std::unique_ptr<mwnet_mt::LogFile> g_logFile;

void outputFunc(const char* msg, int len)
{
  g_logFile->append(msg, len);
}

void flushFunc()
{
  g_logFile->flush();
}
//........................................................

int main(int argc, char* argv[])
{
	int bPrintLog = 1;
	int post_get = 1;
	int totalsend = 1;
	int max_conn = 1000;

	printf("PrintLog? 1: print to console 0: output to file \n");
	scanf("%d", &bPrintLog);

	printf("1:post  2:get\n");
	scanf("%d", &post_get);

	printf("max conns(<=50000):\n");
	scanf("%d", &max_conn);
	max_conn>50000?max_conn=50000:1;

	printf("1: keepalive 0: close\n");
	scanf("%d", &g_nKeepAlive);

	printf("test loop cnt:\n");
	scanf("%d", &totalsend);

	printf("input http url:\n");
	scanf("%s", g_szHttpUrl);

	printf("input http body:\n");
	scanf("%s", g_szHttpBody);

	if (!bPrintLog)
	{
		mkdir("./curltestlogs", 0755);
		if (!g_logFile) g_logFile.reset(new mwnet_mt::LogFile("./curltestlogs/", "curltestlog", 1024*1024*1024, true, 5, 1024));
	  	mwnet_mt::Logger::setOutput(outputFunc);
	  	mwnet_mt::Logger::setFlush(flushFunc);
	}

	//初始化httpclient
	http_cli.InitHttpClient(NULL, func_onmsg_cb, 4, max_conn, max_conn);
	//初始化并启动发送线程
	ThreadPoolPtr pPool(new mwnet_mt::ThreadPool("testcurl"));
	pPool->setMaxQueueSize(20000);
	pPool->start(4);
	
	//loop test.......
	int ttmp = totalsend;
	time_t t1 = time(NULL);
	time_t t2 = t1;
	while (ttmp--)
	{
		// loop add threadpool
		pPool->run(std::bind(test, post_get, g_szHttpUrl, g_szHttpBody));
		
		// 实时速度 3秒一次
		if ((time(NULL)-t2) >= 3) 
		{
			if (!bPrintLog) printf("now,totalcnt:%d,waitsndcnt:%d,waitrspcnt:%d,spd:%d\n", 
									totalsend-ttmp, 
									http_cli.GetWaitReqCnt(),
									http_cli.GetWaitRspCnt(), 
									static_cast<int>((totalsend-ttmp)/(time(NULL)-t1))*1);
			
			t2 = time(NULL);
		}
	}
	
	// 速度汇总
	if ((time(NULL)-t1) > 0) printf("end,totalcnt:%d,spd:%d\n", totalsend, static_cast<int>(totalsend/(time(NULL)-t1))*1);
	
	sleep(30);

	//goto AGAIN;
		
	return 0;
}

