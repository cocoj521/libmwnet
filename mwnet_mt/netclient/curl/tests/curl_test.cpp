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
"<userId>J62003</userId>"
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

typedef std::function<void ()> THREAD_TASK;

using namespace MWNET_MT;
using namespace MWNET_MT::CURL_HTTPCLIENT;

int func_onmsg_cb(void* pInvoker, uint64_t req_uuid, const boost::any& params, const HttpResponsePtr& response)
{
	//if (response->GetErrCode() != 0 || response->GetReqTotalConsuming() >= 1000*1000)
	{
		LOG_INFO 
				 << "URL:" << response->GetEffectiveUrl() << ",code:" << response->GetErrCode() << ",desc:" << response->GetErrDesc() 
			     << "\n" << "ResponseCode:" << response->GetRspCode() 
			     << " totaltm:" << response->GetReqTotalConsuming() << "us"
			     << " conntm:" << response->GetReqConnectConsuming() << "us"
			     << " nameloopuptm:" << response->GetReqNameloopupConsuming() << "us"
			     << " reqinquetm:"<<response->GetReqInQueTime() << "us"
				 << " reqsdtm:" <<response->GetReqSendTime() << "us"
			     << " rsprecvtm:" << response->GetRspRecvTime() << "us"
			     << "\n" << response->GetHeader() << response->GetBody();
	}
	return 0;
}
CurlHttpClient http_cli;

void test(const std::string& strUrlTest)
{
	std::string strBodyTmp = strBody;
	HttpRequestPtr http_request = http_cli.GetHttpRequest(strUrlTest, HTTP_1_1, HTTP_REQUEST_POST, false);
	if (http_request)
	{
		LOG_DEBUG << "GetHttpRequest:" << http_request->GetReqUUID();

		
		http_request->SetTimeOut(30, 60);
		//http_request->SetHeader("myheader1", "nothing1");
		//http_request->SetHeader("myheader2", "nothing2");
		//http_request->SetContentType("text/json");
		http_request->SetBody(strBodyTmp);
		
		boost::any params;
		while (0 != http_cli.SendHttpRequest(params, http_request))
		{
			LOG_INFO << "-=-=-=-=-=-=-=SendHttpRequest Error-=-=-=-==-=";
			usleep(1000*1000);
		}
		
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
	int bRunOnce  = 0;
	int bHttps = 1;

	printf("PrintLog? 1: print to console 0: output to file \n");
	scanf("%d", &bPrintLog);

	printf("RunOnce? 1: only run one time 0: run loop... \n");
	scanf("%d", &bRunOnce);

	printf("https? 4:jd_http_test 3:jd_https_test 2: both 1: https request 0: http request \n");
	scanf("%d", &bHttps);

	if (!bPrintLog)
	{
		//暂时取消
		mkdir("./curltestlogs", 0755);
		if (!g_logFile) g_logFile.reset(new mwnet_mt::LogFile("./curltestlogs/", "curltestlog", 1024*1024*1024, true, 5, 1024));
	  	mwnet_mt::Logger::setOutput(outputFunc);
	  	mwnet_mt::Logger::setFlush(flushFunc);
	}
	http_cli.InitHttpClient(NULL, func_onmsg_cb, true, 4, 20000, 3000);
	ThreadPoolPtr pPool(new mwnet_mt::ThreadPool("testcurl"));
	pPool->setMaxQueueSize(20000);
	pPool->start(4);
	/*
	HttpRequestPtr http_request = http_cli.GetHttpRequest(url);
	http_request->SetBody(strBody);
	http_request->SetUserAgent("curl");
	http_request->SetContentType("text/json");
	http_request->SetConnection("Close");
	http_request->SetTimeOut(2, 5);
	http_request->SetHeader("myheader", "nothing");
	boost::any params;
	http_cli.SendHttpRequest(params, http_request);
	*/

	if (bRunOnce)
	{
		if (0 == bHttps)
		{
			pPool->run(std::bind(test, http_url));
		}
		else if (1 == bHttps)
		{
			pPool->run(std::bind(test, https_url));
		}
		else if (2 == bHttps)
		{
			pPool->run(std::bind(test, http_url));
			pPool->run(std::bind(test, https_url));
		}
		else if (3 == bHttps)
		{
			pPool->run(std::bind(test, https_jdtest));
		}
		else if (4 == bHttps)
		{
			pPool->run(std::bind(test, http_jdtest));
		}
		sleep(3600);
		return 0;
	}

//AGAIN:	
	int totalsend = 100000000;
	int ttmp = totalsend;
	time_t t1 = time(NULL);
	time_t t2 = t1;
	while (ttmp--)
	{
		if (0 == bHttps)
		{
			pPool->run(std::bind(test, http_url));
		}
		else if (1 == bHttps)
		{
			pPool->run(std::bind(test, https_url));
		}
		else if (2 == bHttps)
		{
			pPool->run(std::bind(test, http_url));
			pPool->run(std::bind(test, https_url));
		}
		else if (3 == bHttps)
		{
			pPool->run(std::bind(test, https_jdtest));
		}
		else if (4 == bHttps)
		{
			pPool->run(std::bind(test, http_jdtest));
		}
		/*
		if (!pPool->TryRun(std::bind(test, url1)))
		{
			LOG_DEBUG << "task full:" << pPool->queueSize();
			usleep(1000*1000);
		}
		*/
		//pPool->run(std::bind(test, url2));
		/*
		if (!pPool->TryRun(std::bind(test, url2)))
		{
			LOG_DEBUG << "task full:" << pPool->queueSize();
			usleep(1000*1000);
		}
		*/
		
		if ((time(NULL)-t2) >= 3) 
		{
			if (!bPrintLog) printf("now,totalcnt:%d,waitcnt:%d,spd:%d\n", totalsend-ttmp, http_cli.GetWaitRspCnt(), static_cast<int>((totalsend-ttmp)/(time(NULL)-t1))*1);
			
			t2 = time(NULL);
		}
		
		//usleep(1*1000);
	}
	
	if ((time(NULL)-t1) > 0) printf("end,totalcnt:%d,spd:%d\n", totalsend, static_cast<int>(totalsend/(time(NULL)-t1))*1);
	
	//sleep(300);

	//goto AGAIN;
		
	return 0;
}

