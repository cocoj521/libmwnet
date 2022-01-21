#include "../CurlHttpClient.h"
#include <functional>
#include <mwnet_mt/base/LogFile.h>
#include <mwnet_mt/base/Logging.h>
#include <mwnet_mt/base/Logging.h>
#include <mwnet_mt/base/ThreadPool.h>
#include <mwnet_mt/util/MWStringUtil.h>
#include <string>
#include <sys/stat.h>
#include <atomic>
#include <mutex>
#include <time.h>

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

using namespace MWNET_MT;
using namespace MWNET_MT::CURL_HTTPCLIENT;
using namespace MWSTRINGUTIL;

std::atomic<uint64_t> rspErrCnt_;
FILE *g_fp = NULL;
// 回调函数
int func_onmsg_cb(void* pInvoker, uint64_t req_uuid, const boost::any& params, const HttpResponsePtr& response)
{
	//if (response->GetErrCode() != 0 || response->GetRspCode() != 200)
	{
		//++rspErrCnt_;
		LOG_INFO
			<< "URL:" << response->GetEffectiveUrl() << ",code:" << response->GetErrCode() << ",desc:" << response->GetErrDesc()
			<< "\n" << "ResponseCode:" << response->GetRspCode()
			<< " TotalConsuming:" << response->GetReqTotalConsuming() << "ms"
			<< " ConnectConsuming:" << response->GetReqConnectConsuming() << "ms"
			<< " NameloopupConsuming:" << response->GetReqNameloopupConsuming() << "ms"
			<< " ReqInQueTm:" << response->GetReqInQueTime()
			<< " ReqSendTm:" << response->GetReqSendTime()
			<< " RspRecvTm:" << response->GetRspRecvTime()
			<< "\n" << response->GetHeader()
			<< response->GetBody()
			;
	}

	
	return 0;
}

// 全局httpclient对象
typedef std::shared_ptr<CurlHttpClient> CurlHttpClientPtr;
CurlHttpClientPtr http_cli;

std::mutex g_mutex;
int64_t phone = 13400000000;
std::string get_httpurl()
{
	std::lock_guard<std::mutex> lock(g_mutex);
	char szBuf[1024] = { 0 };
	sprintf(szBuf, "http://47.104.170.213:8087/sms/checkblack?username=mengwang&password=150241&checktype=1&timestamp=1622820336000&sign=5BF71DCF18B3007FD262D1536C8C9A98&blacktype=sms&mobile=%ld", phone);
	++phone;
	return std::string(szBuf);
}

std::string get_httpurl(const char* szPhone)
{
	char szBuf[1024] = { 0 };
	sprintf(szBuf, "http://47.104.170.213:8087/sms/checkblack?username=mengwang&password=150241&checktype=1&timestamp=1622820336000&sign=5BF71DCF18B3007FD262D1536C8C9A98&blacktype=sms&mobile=%s", szPhone);
	printf("req:%s\n", szBuf);
	return std::string(szBuf);
}
//http://msg-aim.monyun.cn:9319/ApiService/v1/AddressManage/queryAimAbility
// 线程任务函数
void test(int post_get, const std::string& strHttpUrl, const std::string& strHttpBody)
{

	std::string strBodyTmp = strHttpBody;
	HttpRequestPtr http_request = http_cli->GetHttpRequest(strHttpUrl/*get_httpurl(strHttpUrl.c_str())*/,
														HTTP_1_1, 
														1==post_get?HTTP_REQUEST_POST:HTTP_REQUEST_GET, 
														g_nKeepAlive);
	if (http_request)
	{
		LOG_TRACE << "GetHttpRequest:" << http_request->GetReqUUID();
///////////////// aim 能力 
		time_t timestamp = time(NULL);
		struct tm* tLogin = localtime(&timestamp);

		char szTime[100] = { 0 };
		sprintf(szTime, "%02d%02d%02d%02d%02d", tLogin->tm_mon + 1, tLogin->tm_mday,
			tLogin->tm_hour, tLogin->tm_min, tLogin->tm_sec);
		char szBuf[100] = { 0 };
		sprintf(szBuf, "AP002600000000147258%s", szTime);
		std::string strMd5 = "";
		MWSTRINGUTIL::StringUtil::ToMd5HexString(szBuf, strlen(szBuf), strMd5);
		http_request->SetHeader("account", "AP0026");
		http_request->SetHeader("pwd", strMd5);
		http_request->SetHeader("timestamp", szTime);

		strBodyTmp = "{\"mobiles\":[{\"mobile\":\"38dbeee35c85ca243f41d351a324478c94281d06\",\"custId\":\"123456789\",\"extData\":\"123456789\"}]}";
		//printf("%s,%s\n", szBuf, strMd5.c_str());
////////////////////////////////
		http_request->SetContentType("application/json;charset=UTF-8");

		http_request->SetTimeOut(30, 60);
		//http_request->SetKeepAliveTime();
		//http_request->SetHeader("myheader1", "nothing1");
		//http_request->SetHeader("myheader2", "nothing2");
		//http_request->SetContentType("text/json");
		
		http_request->SetBody(strBodyTmp);
		
		boost::any params(http_request);
		int nRet = http_cli->SendHttpRequest(http_request, params, true);
		if (0 != nRet)
		{
			//LOG_ERROR << "SendHttpRequest Error:" << nRet;
		}
		
		//sleep(5);
		//http_cli.CancelHttpRequest(http_request);
		//LOG_INFO << "-=-=-=-=-=-=-=CancelHttpRequest-=-=-=-==-=";
		//sleep(3600);
		//LOG_TRACE << "SendHttpRequest:" << http_request->GetReqUUID();
	}
	else
	{
		//LOG_ERROR << "GetHttpRequest Error";
	}
}
void testTextMatchSvr(int post_get, const std::string& strHttpUrl, const std::string& strHttpBody)
{
	std::string strBodyTmp = strHttpBody;
	HttpRequestPtr http_request = http_cli->GetHttpRequest(strHttpUrl/*get_httpurl(strHttpUrl.c_str())*/,
		HTTP_1_1,
		1 == post_get ? HTTP_REQUEST_POST : HTTP_REQUEST_GET,
		g_nKeepAlive);
	if (http_request)
	{
		LOG_TRACE << "GetHttpRequest:" << http_request->GetReqUUID();
		///////////////// aim 能力 
		time_t timestamp = time(NULL);
		struct tm* tLogin = localtime(&timestamp);

		char szTime[100] = { 0 };
		sprintf(szTime, "%02d%02d%02d%02d%02d", tLogin->tm_mon + 1, tLogin->tm_mday,
			tLogin->tm_hour, tLogin->tm_min, tLogin->tm_sec);
		char szBuf[100] = { 0 };
		sprintf(szBuf, "RCOSPT00000000QXxqzPiT%s", szTime);
		std::string strMd5 = "";
		MWSTRINGUTIL::StringUtil::ToMd5HexString(szBuf, strlen(szBuf), strMd5);
		MWSTRINGUTIL::StringUtil::FormatString(strBodyTmp, "{\"userId\":\"RCOSPT\",\"pwd\":\"%s\",\"timestamp\":\"%s\",\"bizCode\":[\"0000006\",\"1102\"],\"messageId\":\"2323213213123\",\"textBlock\":[{\"id\":\"1\",\"text\":\"FUCK,FUCK,FUCK,FUCK,FUCK,FUCK,FUCK,FUCK,FUCK,adfdce shiasfce shi324ce shixce shifadfafce shi3242,FUCK,FUCK,FUCK,FUCK,FUCK,FUCK,FUCK,FUCK\"}]}", strMd5.c_str(), szTime);

		//printf("%s,%s\n", szBuf, strMd5.c_str());
		////////////////////////////////
		http_request->SetContentType("application/json;charset=UTF-8");

		http_request->SetTimeOut(30, 60);
		//http_request->SetKeepAliveTime();
		//http_request->SetHeader("myheader1", "nothing1");
		//http_request->SetHeader("myheader2", "nothing2");
		//http_request->SetContentType("text/json");

		http_request->SetBody(strBodyTmp);

		boost::any params(http_request);
		int nRet = http_cli->SendHttpRequest(http_request, params, true);
		if (0 != nRet)
		{
			//LOG_ERROR << "SendHttpRequest Error:" << nRet;
		}

		//sleep(5);
		//http_cli.CancelHttpRequest(http_request);
		//LOG_INFO << "-=-=-=-=-=-=-=CancelHttpRequest-=-=-=-==-=";
		//sleep(3600);
		//LOG_TRACE << "SendHttpRequest:" << http_request->GetReqUUID();
	}
	else
	{
		//LOG_ERROR << "GetHttpRequest Error";
	}
}
typedef std::shared_ptr<mwnet_mt::ThreadPool> ThreadPoolPtr;

//日志
//..........................................................
std::unique_ptr<mwnet_mt::LogFile> g_logFile;

void outputFunc(const char* msg, size_t len)
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
	int64_t totalsend = 1;
	int max_conn = 1000;

	printf("PrintLog? 1: print to console 0: output to file \n");
	scanf("%d", &bPrintLog);

	printf("1:post  2:get\n");
	scanf("%d", &post_get);

	printf("max conns(<=50000):\n");
	scanf("%d", &max_conn);
	max_conn > 50000 ? max_conn = 50000 : 1;

	printf("1: keepalive 0: close\n");
	scanf("%d", &g_nKeepAlive);

	printf("test loop cnt:\n");
	scanf("%ld", &totalsend);

	printf("loop cnt:%ld\n", totalsend);

	printf("input http url:\n");
	scanf("%s", g_szHttpUrl);

	printf("input http body:\n");
	scanf("%s", g_szHttpBody);

	if (!bPrintLog)
	{
		mkdir("./curltestlogs", 0755);
		if (!g_logFile) g_logFile.reset(new mwnet_mt::LogFile("./curltestlogs/", "curltestlog", 1024 * 1024 * 1024, true, 0, 1));
		mwnet_mt::Logger::setLogLevel(mwnet_mt::Logger::LogLevel::INFO);
		mwnet_mt::Logger::setOutput(outputFunc);
		mwnet_mt::Logger::setFlush(flushFunc);
		g_fp = fopen("./blk.txt","a+");
	}

	http_cli.reset(new CurlHttpClient());
	//初始化httpclient
	http_cli->InitHttpClient(NULL, func_onmsg_cb, 8, max_conn, max_conn);
	//初始化并启动发送线程
	ThreadPoolPtr pPool(new mwnet_mt::ThreadPool("testcurl"));
	pPool->setMaxQueueSize(10000);
	pPool->start(8);

	//loop test.......
	int64_t ttmp = 0;
	time_t t1 = time(NULL);
	time_t t2 = t1;
/*
///////
	std::vector<std::string> vPhone;
	// 读文件
	FILE *fp = fopen("./phone.txt", "rb");
	if (fp)
	{
		char szBuf[32 + 1] = { 0 };
		while (fgets(szBuf, 15, fp) != NULL)
		{
			std::string strPhone = "";
			strPhone.assign(szBuf, 11);
			vPhone.push_back(strPhone);
			printf("%ld,%s\n", strPhone.length(), strPhone.c_str());
		}
		fclose(fp);
	}
	//return 0;
	std::vector<std::string>::iterator it = vPhone.begin();
	for (; it != vPhone.end(); ++it)
	{
		// loop add threadpool
		pPool->run(std::bind(test, post_get, it->c_str(), g_szHttpBody));
	}
	printf("over!\n");
	sleep(60);
	return 0;

//////////
*/
	int64_t __last = 0;

	while (1)
	{
	while (ttmp < totalsend)
	{
		// loop add threadpool
		if (pPool->TryRun(std::bind(testTextMatchSvr, post_get, g_szHttpUrl, g_szHttpBody)))
		{
			++ttmp;
			//usleep(1);
		}
		
		// 实时速度 1秒一次
		if ((time(NULL) - t2) >= 1)
		{
			printf("totalCnt:%ld,thrPoolQue:%ld,waitSndCnt:%ld,waitRspCnt:%ld,rspErr:%lu,spd:%ld\n",
				ttmp, pPool->queueSize(),
				http_cli->GetWaitReqCnt(),
				http_cli->GetWaitRspCnt(),
				rspErrCnt_.load(),
				static_cast<int64_t>((ttmp - __last) / (time(NULL) - t2)) * 1);

			t2 = time(NULL);
			__last = ttmp;
		}
	}
	//http_cli->ExitHttpClient();
	if (time(NULL) - t1 > 0)
	{
		printf("totalcnt:%ld,rspErr:%lu,spd:%ld\n",
			totalsend,
			rspErrCnt_.load(),
			static_cast<int64_t>((totalsend) / (time(NULL) - t1)) * 1);
		printf("totalCnt:%ld,thrPoolQue:%ld,waitSndCnt:%ld,waitRspCnt:%ld,rspErr:%lu,spd:%ld\n",
			ttmp, pPool->queueSize(),
			http_cli->GetWaitReqCnt(),
			http_cli->GetWaitRspCnt(),
			rspErrCnt_.load(),
			static_cast<int64_t>((ttmp) / (time(NULL) - t1)) * 1);
	}
	sleep(1);
}

	

	return 0;
}

