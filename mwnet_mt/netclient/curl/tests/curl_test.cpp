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
char g_smsPort[21 + 1] = { 0 };
char g_aimUrl[32 + 1] = { 0 };
char g_smsSign[64 + 1] = { 0 };
char g_szAimAccount[32 + 1] = { 0 };
char g_szAimAccPwd[64 + 1] = { 0 };
int64_t g_nAimPhone = 13500000000;
int g_nKeepAlive = 0;
int g_nConnectTimeout = 3;
int g_nTotalTimeout = 10;
uint32_t g_nMaxDelay = 100;

using namespace MWNET_MT;
using namespace MWNET_MT::CURL_HTTPCLIENT;
using namespace MWSTRINGUTIL;

std::atomic<uint64_t> rspErrCnt_;
std::atomic<uint64_t> nsloolupCnt_1_;
std::atomic<uint64_t> nsloolupCnt_2_;
std::atomic<uint64_t> nsloolupCnt_3_;
std::atomic<uint64_t> nsloolupCnt_4_;
std::atomic<uint64_t> nsloolupCnt_5_;
std::atomic<uint64_t> nsloolupCnt_6_;
std::atomic<uint64_t> nsloolupCnt_7_;
std::atomic<uint64_t> nsloolupCnt_8_;
std::atomic<uint64_t> nsloolupCnt_9_;
std::atomic<uint64_t> connectTimeOutCnt_1_;
std::atomic<uint64_t> connectTimeOutCnt_2_;
std::atomic<uint64_t> connectTimeOutCnt_3_;
std::atomic<uint64_t> connectTimeOutCnt_4_;
std::atomic<uint64_t> connectTimeOutCnt_5_;
std::atomic<uint64_t> connectTimeOutCnt_6_;
std::atomic<uint64_t> connectTimeOutCnt_7_;
std::atomic<uint64_t> connectTimeOutCnt_8_;
std::atomic<uint64_t> connectTimeOutCnt_9_;
std::atomic<uint64_t> rspErrCnt_1_;
std::atomic<uint64_t> rspErrCnt_2_;
std::atomic<uint64_t> rspErrCnt_3_;
std::atomic<uint64_t> rspErrCnt_4_;
std::atomic<uint64_t> rspErrCnt_5_;
std::atomic<uint64_t> rspErrCnt_6_;
std::atomic<uint64_t> rspErrCnt_7_;
std::atomic<uint64_t> rspErrCnt_8_;
std::atomic<uint64_t> rspErrCnt_9_;
FILE *g_fp = NULL;
// 回调函数
int func_onmsg_cb(void* pInvoker, uint64_t req_uuid, const boost::any& params, const HttpResponsePtr& response)
{
//err
// 	if ((response->GetErrCode() != 0 || response->GetRspCode() != 200)
// 		|| (response->GetRspCode() == 200 && response->GetErrCode() == 0)
// 		)
	{
		
		if (response->GetErrCode() != 0 || response->GetRspCode() != 200)
		{
			++rspErrCnt_;
			LOG_INFO
				<< "ErrReq ReqUUID:" << req_uuid << ",code:" << response->GetErrCode() << ",desc:" << response->GetErrDesc()
				<< ",TotalConsuming:" << response->GetReqTotalConsuming() << "ms"
				<< ",ConnectConsuming:" << response->GetReqConnectConsuming() << "ms"
				<< ",NameloopupConsuming:" << response->GetReqNameloopupConsuming() << "ms"
				<< ",ReqInQueTm:" << response->GetReqInQueTime()
				<< ",ReqSendTm:" << response->GetReqSendTime()
				<< ",RspRecvTm:" << response->GetRspRecvTime()
				<< "\n" << response->GetHeader()
				<< response->GetBody();

		}
		uint32_t nSpan = response->GetReqNameloopupConsuming();
		if (nSpan < 50)
		{
			++nsloolupCnt_1_;
		}
		else if (nSpan >= 50 && nSpan < 100)
		{
			++nsloolupCnt_2_;
		}
		else if (nSpan >= 100 && nSpan < 150)
		{
			++nsloolupCnt_3_;
		}
		else if (nSpan >= 150 && nSpan < 200)
		{
			++nsloolupCnt_4_;
		}
		else if (nSpan >= 200 && nSpan < 300)
		{
			++nsloolupCnt_5_;
		}
		else if (nSpan >= 300 && nSpan < 500)
		{
			++nsloolupCnt_6_;
		}
		else if (nSpan >= 500 && nSpan < 1000)
		{
			++nsloolupCnt_7_;
		}
		else if (nSpan >= 1000 && nSpan < 2000)
		{
			++nsloolupCnt_8_;
		}
		else if (nSpan >= 2000)
		{
			++nsloolupCnt_9_;
		}
		nSpan = response->GetReqConnectConsuming();
		if (nSpan < 50)
		{
			++connectTimeOutCnt_1_;
		}
		else if (nSpan >= 50 && nSpan < 100)
		{
			++connectTimeOutCnt_2_;
		}
		else if (nSpan >= 100 && nSpan < 150)
		{
			++connectTimeOutCnt_3_;
		}
		else if (nSpan >= 150 && nSpan < 200)
		{
			++connectTimeOutCnt_4_;
		}
		else if (nSpan >= 200 && nSpan < 300)
		{
			++connectTimeOutCnt_5_;
		}
		else if (nSpan >= 300 && nSpan < 500)
		{
			++connectTimeOutCnt_6_;
		}
		else if (nSpan >= 500 && nSpan < 1000)
		{
			++connectTimeOutCnt_7_;
		}
		else if (nSpan >= 1000 && nSpan < 2000)
		{
			++connectTimeOutCnt_8_;
		}
		else if (nSpan >= 2000)
		{
			++connectTimeOutCnt_9_;
		}

		nSpan = response->GetReqTotalConsuming();
		if (nSpan < 50)
		{
			++rspErrCnt_1_;
		}
		else if (nSpan >= 50 && nSpan < 100)
		{
			++rspErrCnt_2_;
		}
		else if (nSpan >= 100 && nSpan < 150)
		{
			++rspErrCnt_3_;
		}
		else if (nSpan >= 150 && nSpan < 200)
		{
			++rspErrCnt_4_;
		}
		else if (nSpan >= 200 && nSpan < 300)
		{
			++rspErrCnt_5_;
		}
		else if (nSpan >= 300 && nSpan < 500)
		{
			++rspErrCnt_6_;
		}
		else if (nSpan >= 500 && nSpan < 1000)
		{
			++rspErrCnt_7_;
		}
		else if (nSpan >= 1000 && nSpan < 2000)
		{
			++rspErrCnt_8_;
		}
		else if (nSpan >= 2000)
		{
			++rspErrCnt_9_;
		}
		LOG_INFO
			<< "ReqUUID:" << req_uuid << ",code:" << response->GetErrCode() << ",desc:" << response->GetErrDesc()
			<< ",TotalConsuming:" << response->GetReqTotalConsuming() << "ms"
			<< ",ConnectConsuming:" << response->GetReqConnectConsuming() << "ms"
			<< ",NameloopupConsuming:" << response->GetReqNameloopupConsuming() << "ms"
			<< ",ReqInQueTm:" << response->GetReqInQueTime()
			<< ",ReqSendTm:" << response->GetReqSendTime()
			<< ",RspRecvTm:" << response->GetRspRecvTime()
			<< ",bDelay:" << (response->GetReqTotalConsuming()>g_nMaxDelay?1:0)
			<< "\n" << response->GetHeader()
			<< response->GetBody()
			;
		/*LOG_INFO
			<< "URL:" << response->GetEffectiveUrl() << ",ReqUUID:" << req_uuid << ",code:" << response->GetErrCode() << ",desc:" << response->GetErrDesc()
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
			*/
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
void testMnps(int post_get, const std::string& strHttpUrl, const std::string& strHttpBody)
{
	std::string strBodyTmp = strHttpBody;
	HttpRequestPtr http_request = http_cli->GetHttpRequest(strHttpUrl,
		HTTP_1_1,
		1 == post_get ? HTTP_REQUEST_POST : HTTP_REQUEST_GET,
		g_nKeepAlive);
	if (http_request)
	{
		///////////////// aim 能力 
		time_t timestamp = time(NULL);
		struct tm* tLogin = localtime(&timestamp);

		char szTime[100] = { 0 };
		sprintf(szTime, "%02d%02d%02d%02d%02d", tLogin->tm_mon + 1, tLogin->tm_mday,
			tLogin->tm_hour, tLogin->tm_min, tLogin->tm_sec);
		char szBuf[100] = { 0 };
		sprintf(szBuf, "CMSABS00000000Fx1VEn8cefRbjhwJQO9rLlhXvHwR94Em%s", szTime);
		std::string strMd5 = "";
		MWSTRINGUTIL::StringUtil::ToMd5HexString(szBuf, strlen(szBuf), strMd5);
		http_request->SetHeader("Userid", "CMSABS");
		http_request->SetHeader("Pwd", strMd5);
		http_request->SetHeader("Timestamp", szTime);

		strBodyTmp = "{\"requestId\":\"56asdd-47503-abce\",\"proType\":\"RMS\",\"msgType\":\"RPT\",\"ecid\":500001,\"apiAcct\":\"TEST01\",\"pushUrl\":\"http://192.169.0.231:1234/sms/v2/std/single_send\",\"messages\":[{\"msgId\":\"2177343094593642363\",\"msglevel\":\"5\",\"payload\":{\"body\":\"hello\"},\"originTime\":\"20220125190012\",\"validTime\":\"20220225190012\"}]}";
		//strBodyTmp = "{\"requestId\":\"56asdd-47503-abce\",\"items\":[{\"proType\":\"RMS\",\"msgType\":\"RPT\",\"ecid\":500001,\"apiAcct\":\"TEST01\",\"apiName\":\"test\",\"sendMode\":\"PUSH\",\"pushRule\": {\"common\": {\"pushVersion\": \"1\",\"pushUrl\": \"http://test.com/rpt/callback\",\"auth_accType\": \"MW\",\"retry_onOff\":\"ON\"}}}]}";
		//printf("%s,%s\n", szBuf, strMd5.c_str());
		////////////////////////////////
		http_request->SetContentType("application/json;charset=UTF-8");
		http_request->SetTimeOut(g_nConnectTimeout, g_nTotalTimeout);
		http_request->SetBody(strBodyTmp);
		boost::any params;
		int nRet = http_cli->SendHttpRequest(http_request, params, true);
		if (0 != nRet)
		{
			LOG_ERROR << "SendHttpRequest Error:" << nRet;
		}
	}
}
void testNormal(int post_get, const std::string& strHttpUrl, const std::string& strHttpBody)
{
	std::string strBodyTmp = strHttpBody;
	HttpRequestPtr http_request = http_cli->GetHttpRequest(strHttpUrl,
														HTTP_1_1,
														1 == post_get ? HTTP_REQUEST_POST : HTTP_REQUEST_GET,
														g_nKeepAlive);
	if (http_request)
	{
		LOG_DEBUG << "GetHttpRequest:" << http_request->GetReqUUID();

		time_t timestamp = time(NULL);
		struct tm* tLogin = localtime(&timestamp);

		char szTime[100] = { 0 };
		sprintf(szTime, "%02d%02d%02d%02d%02d", tLogin->tm_mon + 1, tLogin->tm_mday,
			tLogin->tm_hour, tLogin->tm_min, tLogin->tm_sec);
		char szBuf[100] = { 0 };
		sprintf(szBuf, "CMSABS00000000Fx1VEn8cefRbjhwJQO9rLlhXvHwR94Em%s", szTime);
		std::string strMd5 = "";
		MWSTRINGUTIL::StringUtil::ToMd5HexString(szBuf, strlen(szBuf), strMd5);
		//http_request->SetHeader("Userid", "CMSABS");
		//http_request->SetHeader("Pwd", strMd5);
		//http_request->SetHeader("Timestamp", szTime);
		http_request->SetHeader("appid", "101080723");
		http_request->SetHeader("timestamp","2021-1-13 11:11:13");
		http_request->SetHeader("nonce", "123");
		http_request->SetHeader("sign", "6edb4aff75a95ace2d948ee5c6bb2b14cff303a45788bf888b020b5d2ff3c6e5");
		http_request->SetHeader("msgTraceId", "jianghb_test_" + std::to_string(http_request->GetReqUUID()));
		http_request->SetHeader("traceid", "jianghb_888");
		http_request->SetContentType("application/json");
		http_request->SetTimeOut(g_nConnectTimeout, g_nTotalTimeout);
		http_request->SetBody(strBodyTmp);
		boost::any params;
		int nRet = http_cli->SendHttpRequest(http_request, params, true);
		if (0 != nRet)
		{
			LOG_ERROR << "SendHttpRequest Error:" << nRet;
		}
	}
}

void testAimPhoneAbility(const std::string& strHttpUrl, const std::string& strAimAccount, const std::string& strAimAccPwd, const int64_t nAimPhone)
{
	HttpRequestPtr http_request = http_cli->GetHttpRequest(strHttpUrl,
		HTTP_1_1,
		HTTP_REQUEST_POST,
		g_nKeepAlive);
	if (http_request)
	{
		LOG_DEBUG << "GetHttpRequest:" << http_request->GetReqUUID();

		time_t timestamp = time(NULL);
		struct tm* tLogin = localtime(&timestamp);

		char szTime[100] = { 0 };
		sprintf(szTime, "%02d%02d%02d%02d%02d", tLogin->tm_mon + 1, tLogin->tm_mday,
			tLogin->tm_hour, tLogin->tm_min, tLogin->tm_sec);
		char szEncPwd[100] = { 0 };
		sprintf(szEncPwd, "%s00000000%s%s", strAimAccount.c_str(), strAimAccPwd.c_str(), szTime);
		std::string strMd5Pwd = "";
		MWSTRINGUTIL::StringUtil::ToMd5HexString(szEncPwd, strlen(szEncPwd), strMd5Pwd);
		http_request->SetHeader("account", strAimAccount);
		http_request->SetHeader("timestamp", szTime);
		http_request->SetHeader("pwd", strMd5Pwd);
		http_request->SetContentType("application/json");
		http_request->SetTimeOut(g_nConnectTimeout, g_nTotalTimeout);
		std::string strPhoneSHA1 = "";
		MWSTRINGUTIL::StringUtil::ToSha1HexString(std::to_string(nAimPhone), strPhoneSHA1);
		std::string strBodyTmp = "";
		MWSTRINGUTIL::StringUtil::FormatString(strBodyTmp,
			"{\"mobiles\":[{\"mobile\":\"%s\",\"custId\":\"123456789012345678901234567890\",\"extData\":\"123456789012345678901234567890\"}]}",
			strPhoneSHA1.c_str());
		http_request->SetBody(strBodyTmp);
		boost::any params;
		int nRet = http_cli->SendHttpRequest(http_request, params, true);
		if (0 != nRet)
		{
			LOG_ERROR << "SendHttpRequest Error:" << nRet;
		}
	}
}

void testOldFacPull(int post_get, const std::string& strHttpUrl, const char* szAimUrl, const char* szSmsPort, const char* szSmsSign)
{
	HttpRequestPtr http_request = http_cli->GetHttpRequest(strHttpUrl,
		HTTP_1_1,
		1 == post_get ? HTTP_REQUEST_POST : HTTP_REQUEST_GET,
		g_nKeepAlive);
	if (http_request)
	{
		LOG_DEBUG << "GetHttpRequest:" << http_request->GetReqUUID();

		time_t timestamp = time(NULL);
		struct tm* tLogin = localtime(&timestamp);

		char szTime[100] = { 0 };
		sprintf(szTime, "%02d%02d%02d%02d%02d", tLogin->tm_mon + 1, tLogin->tm_mday,
			tLogin->tm_hour, tLogin->tm_min, tLogin->tm_sec);
		char szBuf[100] = { 0 };
		sprintf(szBuf, "CMSABS00000000Fx1VEn8cefRbjhwJQO9rLlhXvHwR94Em%s", szTime);
		std::string strMd5 = "";
		MWSTRINGUTIL::StringUtil::ToMd5HexString(szBuf, strlen(szBuf), strMd5);
		//http_request->SetHeader("Userid", "CMSABS");
		//http_request->SetHeader("Pwd", strMd5);
		//http_request->SetHeader("Timestamp", szTime);
		http_request->SetHeader("appid", "101080723");
		http_request->SetHeader("timestamp", "2021-1-13 11:11:13");
		http_request->SetHeader("nonce", "123");
		http_request->SetHeader("sign", "6edb4aff75a95ace2d948ee5c6bb2b14cff303a45788bf888b020b5d2ff3c6e5");
		http_request->SetHeader("msgTraceId", "jianghb_test_" + std::to_string(http_request->GetReqUUID()));
		http_request->SetHeader("traceid", "jianghb_888");
		http_request->SetContentType("application/json");
		http_request->SetTimeOut(g_nConnectTimeout, g_nTotalTimeout);

		char szBody[1024 + 1] = { 0 };
		sprintf(szBody, "{\"aimUrl\":\"%s\",\"callSource\":0,\"simId\":\"%ld\",\"smsId\":\"%ld\",\"port\":\"%s\",\"oaId\":\"123456789\",\"smsSigns\":[\"%s\"]}",
			szAimUrl, http_request->GetReqUUID(), http_request->GetReqUUID(), szSmsPort, szSmsSign);
		std::string strBodyTmp(szBody);
		http_request->SetBody(strBodyTmp);
		boost::any params;
		int nRet = http_cli->SendHttpRequest(http_request, params, true);
		if (0 != nRet)
		{
			LOG_ERROR << "SendHttpRequest Error:" << nRet;
		}
	}
}

void testNewFacPull(int post_get, const std::string& strHttpUrl, const char* szAimUrl, const char* szSmsPort, const char* szSmsSign)
{
	HttpRequestPtr http_request = http_cli->GetHttpRequest(strHttpUrl,
		HTTP_1_1,
		1 == post_get ? HTTP_REQUEST_POST : HTTP_REQUEST_GET,
		g_nKeepAlive);
	if (http_request)
	{
		LOG_DEBUG << "GetHttpRequest:" << http_request->GetReqUUID();

		time_t timestamp = time(NULL);
		struct tm* tLogin = localtime(&timestamp);

		char szTime[100] = { 0 };
		sprintf(szTime, "%02d%02d%02d%02d%02d", tLogin->tm_mon + 1, tLogin->tm_mday,
			tLogin->tm_hour, tLogin->tm_min, tLogin->tm_sec);
		char szBuf[100] = { 0 };
		sprintf(szBuf, "AP000400000000565326288923942912VCvckp%s", szTime);
		std::string strMd5 = "";
		MWSTRINGUTIL::StringUtil::ToMd5HexString(szBuf, strlen(szBuf), strMd5);
		http_request->SetHeader("account", "AP0004");
		http_request->SetHeader("pwd", strMd5);
		http_request->SetHeader("timestamp", szTime);
		http_request->SetHeader("ftpye", "HuaWei");
		http_request->SetContentType("application/json");
		http_request->SetTimeOut(g_nConnectTimeout, g_nTotalTimeout);

		char szBody[1024+1] = { 0 };
		sprintf(szBody, "{\"aimUrl\":\"%s\",\"callSource\":0,\"simId\":\"%ld\",\"smsId\":\"%ld\",\"port\":\"%s\",\"oaId\":\"123456789\",\"smsSigns\":[\"%s\"]}", 
			szAimUrl, http_request->GetReqUUID(), http_request->GetReqUUID(), szSmsPort, szSmsSign);
		std::string strBodyTmp(szBody);
		http_request->SetBody(strBodyTmp);
		boost::any params;
		int nRet = http_cli->SendHttpRequest(http_request, params, true);
		if (0 != nRet)
		{
			LOG_ERROR << "SendHttpRequest Error:" << nRet;
		}
	}
}
//http://msg-aim.monyun.cn:9319/ApiService/v1/AddressManage/queryAimAbility
// 线程任务函数
void testAim(int post_get, const std::string& strHttpUrl, const std::string& strHttpBody)
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
		sprintf(szBuf, "RCOSPT00000000QXxqzPiT%s", szTime);
		std::string strMd5 = "";
		MWSTRINGUTIL::StringUtil::ToMd5HexString(szBuf, strlen(szBuf), strMd5);
		http_request->SetHeader("account", "RCOSPT");
		http_request->SetHeader("pwd", strMd5);
		http_request->SetHeader("timestamp", szTime);

		strBodyTmp = "{\"mobiles\":[{\"mobile\":\"38dbeee35c85ca243f41d351a324478c94281d06\",\"custId\":\"123456789\",\"extData\":\"123456789\"}]}";
		//printf("%s,%s\n", szBuf, strMd5.c_str());
////////////////////////////////
		http_request->SetContentType("application/json;charset=UTF-8");

		http_request->SetTimeOut(g_nConnectTimeout, g_nTotalTimeout);
		//http_request->SetKeepAliveTime();
		//http_request->SetHeader("myheader1", "nothing1");
		//http_request->SetHeader("myheader2", "nothing2");
		//http_request->SetContentType("text/json");
		
		http_request->SetBody(strBodyTmp);
		
		boost::any params;
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

		http_request->SetTimeOut(g_nConnectTimeout, g_nTotalTimeout);
		//http_request->SetKeepAliveTime();
		//http_request->SetHeader("myheader1", "nothing1");
		//http_request->SetHeader("myheader2", "nothing2");
		//http_request->SetContentType("text/json");

		http_request->SetBody(strBodyTmp);

		boost::any params;
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
	int nMode = 1;
	int nFacPullType = 1;
	std::atomic<int64_t> nAimPhone;

	printf("select test mode? 1: normal test 2: test cmcc facpull 3: test aimPhoneAbility\n");
	scanf("%d", &nMode);

	printf("PrintLog? 1: print to console 0: output to file \n");
	scanf("%d", &bPrintLog);

	printf("1:post  2:get\n");
	scanf("%d", &post_get);

	printf("max conns(<=50000):\n");
	scanf("%d", &max_conn);
	max_conn > 50000 ? max_conn = 50000 : 1;

	printf("1: keepalive 0: close\n");
	scanf("%d", &g_nKeepAlive);

	printf("connect timeout(s):\n");
	scanf("%d", &g_nConnectTimeout);

	printf("total timeout(s):\n");
	scanf("%d", &g_nTotalTimeout);

	printf("test loop cnt:\n");
	scanf("%ld", &totalsend);

	int nIoThr = 8;
	printf("IoThrNum:\n");
	scanf("%d", &nIoThr);

	int nWorkThr = 8;
	printf("WorkThrNum:\n");
	scanf("%d", &nWorkThr);

	int nThrQueSize = 1000;
	printf("WorkThrQueSize:\n");
	scanf("%d", &nThrQueSize);

	int nLogLevel = 2;
	printf("LogLevel 0-TRACE 1-DEBUG 2-INFO:\n");
	scanf("%d", &nLogLevel);

	// cmcc facpull
	if (2 == nMode)
	{
		printf("facpullType 1-old facpull 2-new facpull:\n");
		scanf("%d", &nFacPullType);

		printf("input http url:\n");
		scanf("%s", g_szHttpUrl);

		printf("input aimUrl:\n");
		scanf("%s", g_aimUrl);

		printf("input smsPort:\n");
		scanf("%s", g_smsPort);

		printf("input smsSign:\n");
		scanf("%s", g_smsSign);
	}
	else if (3 == nMode)
	{
		printf("input http url(/ApiService/v1/AddressManage/queryAimAbility):\n");
		scanf("%s", g_szHttpUrl);

		printf("input account(AP0613):\n");
		scanf("%s", g_szAimAccount);

		printf("input pwd:\n");
		scanf("%s", g_szAimAccPwd);

		printf("input phone(8613500000000,auto inc):\n");
		scanf("%ld", &g_nAimPhone);
		nAimPhone = g_nAimPhone;

		printf("input maxRecordDelay(100ms):\n");
		scanf("%d", &g_nMaxDelay);
	}
	else
	{
		printf("input http url:\n");
		scanf("%s", g_szHttpUrl);

		printf("input http body:\n");
		scanf("%s", g_szHttpBody);
	}

	if (!bPrintLog)
	{
		mkdir("./curltestlogs", 0755);
		if (!g_logFile) g_logFile.reset(new mwnet_mt::LogFile("./curltestlogs/", "curltestlog", 1024 * 1024 * 1024, true, 0, 1));
		if (0 == nLogLevel)
		{
			mwnet_mt::Logger::setLogLevel(mwnet_mt::Logger::LogLevel::TRACE);
		}
		else if (1 == nLogLevel)
		{
			mwnet_mt::Logger::setLogLevel(mwnet_mt::Logger::LogLevel::DEBUG);
		}
		else if (2 == nLogLevel)
		{
			mwnet_mt::Logger::setLogLevel(mwnet_mt::Logger::LogLevel::INFO);
		}
		mwnet_mt::Logger::setOutput(outputFunc);
		mwnet_mt::Logger::setFlush(flushFunc);
		g_fp = fopen("./blk.txt","a+");
	}

	http_cli.reset(new CurlHttpClient());
	//初始化httpclient
	http_cli->InitHttpClient(NULL, func_onmsg_cb, nIoThr, max_conn, max_conn);
	//初始化并启动发送线程
	ThreadPoolPtr pPool(new mwnet_mt::ThreadPool("testcurl"));
	pPool->setMaxQueueSize(nThrQueSize);
	pPool->start(nWorkThr);

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
		if (1 == nMode)
		{
			// loop add threadpool
			pPool->run(std::bind(testNormal, post_get, g_szHttpUrl, g_szHttpBody));
		}
		// cmcc facpull
		else if (2 == nMode)
		{
			// old
			if (1 == nFacPullType)
			{
				// loop add threadpool
				pPool->run(std::bind(testOldFacPull, post_get, g_szHttpUrl, g_aimUrl, g_smsPort,  g_smsSign));
			}
			// new
			else if (2 == nFacPullType)
			{
				// loop add threadpool
				pPool->run(std::bind(testNewFacPull, post_get, g_szHttpUrl, g_aimUrl, g_smsPort, g_smsSign));
			}
		}
		// aim phone ability
		else if (3 == nMode)
		{
			++nAimPhone;
			pPool->run(std::bind(testAimPhoneAbility, g_szHttpUrl, g_szAimAccount, g_szAimAccPwd, nAimPhone.load()));
		}

		++ttmp;

		// 实时速度 1秒一次
		if ((time(NULL) - t2) >= 1)
		{
			printf("totalCnt:%ld,thrPoolQue:%ld,waitSndCnt:%ld,waitRspCnt:%ld,rspErrCnt:%lu,QPS:%ld\n"
				"----------timeCost(nslookupTime/connectTime/totalTime/percent)----------\n"
				"0-50ms     :%9lu/%.4f/%9lu/%.4f/%9lu/%.4f\n"
				"50-100ms   :%9lu/%.4f/%9lu/%.4f/%9lu/%.4f\n"
				"100-150ms  :%9lu/%.4f/%9lu/%.4f/%9lu/%.4f\n"
				"150-200ms  :%9lu/%.4f/%9lu/%.4f/%9lu/%.4f\n"
				"200-300ms  :%9lu/%.4f/%9lu/%.4f/%9lu/%.4f\n"
				"300-500ms  :%9lu/%.4f/%9lu/%.4f/%9lu/%.4f\n"
				"500-1000ms :%9lu/%.4f/%9lu/%.4f/%9lu/%.4f\n"
				"1000-2000ms:%9lu/%.4f/%9lu/%.4f/%9lu/%.4f\n"
				"2000-....ms:%9lu/%.4f/%9lu/%.4f/%9lu/%.4f\n",
				ttmp, pPool->queueSize(),
				http_cli->GetWaitReqCnt(),
				http_cli->GetWaitRspCnt(),
				rspErrCnt_.load(),
				static_cast<int64_t>((ttmp - __last) / (time(NULL) - t2)) * 1,
				nsloolupCnt_1_.load(), static_cast<double>(nsloolupCnt_1_.load()) / static_cast<double>(ttmp), connectTimeOutCnt_1_.load(), static_cast<double>(connectTimeOutCnt_1_.load()) / static_cast<double>(ttmp), rspErrCnt_1_.load(), static_cast<double>(rspErrCnt_1_.load()) / static_cast<double>(ttmp),
				nsloolupCnt_2_.load(), static_cast<double>(nsloolupCnt_2_.load()) / static_cast<double>(ttmp), connectTimeOutCnt_2_.load(), static_cast<double>(connectTimeOutCnt_2_.load()) / static_cast<double>(ttmp), rspErrCnt_2_.load(), static_cast<double>(rspErrCnt_2_.load()) / static_cast<double>(ttmp),
				nsloolupCnt_3_.load(), static_cast<double>(nsloolupCnt_3_.load()) / static_cast<double>(ttmp), connectTimeOutCnt_3_.load(), static_cast<double>(connectTimeOutCnt_3_.load()) / static_cast<double>(ttmp), rspErrCnt_3_.load(), static_cast<double>(rspErrCnt_3_.load()) / static_cast<double>(ttmp),
				nsloolupCnt_4_.load(), static_cast<double>(nsloolupCnt_4_.load()) / static_cast<double>(ttmp), connectTimeOutCnt_4_.load(), static_cast<double>(connectTimeOutCnt_4_.load()) / static_cast<double>(ttmp), rspErrCnt_4_.load(), static_cast<double>(rspErrCnt_4_.load()) / static_cast<double>(ttmp),
				nsloolupCnt_5_.load(), static_cast<double>(nsloolupCnt_5_.load()) / static_cast<double>(ttmp), connectTimeOutCnt_5_.load(), static_cast<double>(connectTimeOutCnt_5_.load()) / static_cast<double>(ttmp), rspErrCnt_5_.load(), static_cast<double>(rspErrCnt_5_.load()) / static_cast<double>(ttmp),
				nsloolupCnt_6_.load(), static_cast<double>(nsloolupCnt_6_.load()) / static_cast<double>(ttmp), connectTimeOutCnt_6_.load(), static_cast<double>(connectTimeOutCnt_6_.load()) / static_cast<double>(ttmp), rspErrCnt_6_.load(), static_cast<double>(rspErrCnt_6_.load()) / static_cast<double>(ttmp),
				nsloolupCnt_7_.load(), static_cast<double>(nsloolupCnt_7_.load()) / static_cast<double>(ttmp), connectTimeOutCnt_7_.load(), static_cast<double>(connectTimeOutCnt_7_.load()) / static_cast<double>(ttmp), rspErrCnt_7_.load(), static_cast<double>(rspErrCnt_7_.load()) / static_cast<double>(ttmp),
				nsloolupCnt_8_.load(), static_cast<double>(nsloolupCnt_8_.load()) / static_cast<double>(ttmp), connectTimeOutCnt_8_.load(), static_cast<double>(connectTimeOutCnt_8_.load()) / static_cast<double>(ttmp), rspErrCnt_8_.load(), static_cast<double>(rspErrCnt_8_.load()) / static_cast<double>(ttmp),
				nsloolupCnt_9_.load(), static_cast<double>(nsloolupCnt_9_.load()) / static_cast<double>(ttmp), connectTimeOutCnt_9_.load(), static_cast<double>(connectTimeOutCnt_9_.load()) / static_cast<double>(ttmp), rspErrCnt_9_.load(), static_cast<double>(rspErrCnt_9_.load()) / static_cast<double>(ttmp));
			t2 = time(NULL);
			__last = ttmp;
		}
	}

	sleep(10);

	printf("Done...totalCnt:%ld,rspErr:%lu,QPS:%ld\n"
		"----------timeCost(nslookupTime/connectTime/totalTime/percent)----------\n"
		"0-50ms     :%9lu/%.4f/%9lu/%.4f/%9lu/%.4f\n"
		"50-100ms   :%9lu/%.4f/%9lu/%.4f/%9lu/%.4f\n"
		"100-150ms  :%9lu/%.4f/%9lu/%.4f/%9lu/%.4f\n"
		"150-200ms  :%9lu/%.4f/%9lu/%.4f/%9lu/%.4f\n"
		"200-300ms  :%9lu/%.4f/%9lu/%.4f/%9lu/%.4f\n"
		"300-500ms  :%9lu/%.4f/%9lu/%.4f/%9lu/%.4f\n"
		"500-1000ms :%9lu/%.4f/%9lu/%.4f/%9lu/%.4f\n"
		"1000-2000ms:%9lu/%.4f/%9lu/%.4f/%9lu/%.4f\n"
		"2000-....ms:%9lu/%.4f/%9lu/%.4f/%9lu/%.4f\n",
		ttmp, rspErrCnt_.load(),
		static_cast<int64_t>(ttmp / (time(NULL) - t1)) * 1,
		nsloolupCnt_1_.load(), static_cast<double>(nsloolupCnt_1_.load()) / static_cast<double>(ttmp), connectTimeOutCnt_1_.load(), static_cast<double>(connectTimeOutCnt_1_.load()) / static_cast<double>(ttmp), rspErrCnt_1_.load(), static_cast<double>(rspErrCnt_1_.load()) / static_cast<double>(ttmp),
		nsloolupCnt_2_.load(), static_cast<double>(nsloolupCnt_2_.load()) / static_cast<double>(ttmp), connectTimeOutCnt_2_.load(), static_cast<double>(connectTimeOutCnt_2_.load()) / static_cast<double>(ttmp), rspErrCnt_2_.load(), static_cast<double>(rspErrCnt_2_.load()) / static_cast<double>(ttmp),
		nsloolupCnt_3_.load(), static_cast<double>(nsloolupCnt_3_.load()) / static_cast<double>(ttmp), connectTimeOutCnt_3_.load(), static_cast<double>(connectTimeOutCnt_3_.load()) / static_cast<double>(ttmp), rspErrCnt_3_.load(), static_cast<double>(rspErrCnt_3_.load()) / static_cast<double>(ttmp),
		nsloolupCnt_4_.load(), static_cast<double>(nsloolupCnt_4_.load()) / static_cast<double>(ttmp), connectTimeOutCnt_4_.load(), static_cast<double>(connectTimeOutCnt_4_.load()) / static_cast<double>(ttmp), rspErrCnt_4_.load(), static_cast<double>(rspErrCnt_4_.load()) / static_cast<double>(ttmp),
		nsloolupCnt_5_.load(), static_cast<double>(nsloolupCnt_5_.load()) / static_cast<double>(ttmp), connectTimeOutCnt_5_.load(), static_cast<double>(connectTimeOutCnt_5_.load()) / static_cast<double>(ttmp), rspErrCnt_5_.load(), static_cast<double>(rspErrCnt_5_.load()) / static_cast<double>(ttmp),
		nsloolupCnt_6_.load(), static_cast<double>(nsloolupCnt_6_.load()) / static_cast<double>(ttmp), connectTimeOutCnt_6_.load(), static_cast<double>(connectTimeOutCnt_6_.load()) / static_cast<double>(ttmp), rspErrCnt_6_.load(), static_cast<double>(rspErrCnt_6_.load()) / static_cast<double>(ttmp),
		nsloolupCnt_7_.load(), static_cast<double>(nsloolupCnt_7_.load()) / static_cast<double>(ttmp), connectTimeOutCnt_7_.load(), static_cast<double>(connectTimeOutCnt_7_.load()) / static_cast<double>(ttmp), rspErrCnt_7_.load(), static_cast<double>(rspErrCnt_7_.load()) / static_cast<double>(ttmp),
		nsloolupCnt_8_.load(), static_cast<double>(nsloolupCnt_8_.load()) / static_cast<double>(ttmp), connectTimeOutCnt_8_.load(), static_cast<double>(connectTimeOutCnt_8_.load()) / static_cast<double>(ttmp), rspErrCnt_8_.load(), static_cast<double>(rspErrCnt_8_.load()) / static_cast<double>(ttmp),
		nsloolupCnt_9_.load(), static_cast<double>(nsloolupCnt_9_.load()) / static_cast<double>(ttmp), connectTimeOutCnt_9_.load(), static_cast<double>(connectTimeOutCnt_9_.load()) / static_cast<double>(ttmp), rspErrCnt_9_.load(), static_cast<double>(rspErrCnt_9_.load()) / static_cast<double>(ttmp));

	sleep(3600);
}

	return 0;
}

