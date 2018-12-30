#include "../NetClient.h"
#include <thread>
#include <iostream>
#include <thread>
#include <chrono>

using namespace MWNET_MT::CLIENT;

int onMessage(void* pInvoker, uint64_t conn_uuid, const boost::any& conn, const HttpResponse& httpResponse)
{
	int nCode = httpResponse.GetStatusCode();
	printf("onMessage--%d\n", nCode);
	if (200 != nCode)
	{
		return 0;
	}
	std::string strBody;
	int nUid = static_cast<int>(conn_uuid);
	static int nCount = 0;
	httpResponse.GetResponseBody(strBody);
	printf("conn_uuid=%d,nStatusCode=%d,nCount:%d,strBody:%s\n", nUid, nCode, ++nCount, strBody.c_str());
	return 0;
}

void init()
{
	NetClient::Instance().InitHttpClient(NULL, 1, 1, 1000, &onMessage,4096,4096,100);
}

int main()
{
	MWNET_MT::CLIENT::HttpRequest req;
	std::string str;
	str = "192.169.1.84";
	//str = "14.215.177.38";
	req.SetRequestIp(str);
	req.SetRequestPort(1234);
	req.SetRequestType(HTTP_REQUEST_GET);
	req.SetHost(str);
	req.SetKeepAlive(false);
	req.SetTimeOut(60);
	req.SetUserAgent("Mozilla/5.0 (Windows NT 6.1; rv:2.0.1) Gecko/20100101 Firefox/4.0.1");
// 	req.SetContentType(CONTENT_TYPE_JSON);
// 	str = "{\"accounts\":[\"tyzh07\",\"APIZH1\"],\"sig\":\"741454a477acd089edd145cb8c47bd801821323630a5e17e704f4ca8af19f756\",\"time\":1512640219}";
// 	req.SetContent(str);
// 	//str = "/sms/V2/cstd/didi/pulldisplaynumber?random=4573765";
// 	str = "/sms/v2/std/batch_send";
// 	req.SetUrl(str);
	std::thread t1(&init);
	sleep(1);

	uint64_t nCount = 10;
	std::string line;
	while (std::getline(std::cin, line))
	{
		//usleep(100000);
		int nRet = NetClient::Instance().SendHttpRequest(nCount, nullptr, req);
		printf("NetClient::Instance().SendHttpRequest::%d\n", nRet);
		if (nRet == 0)
		{
			nCount--;
		}	
	}

	int nRet = NetClient::Instance().SendHttpRequest(nCount, nullptr, req);
	printf("NetClient::Instance().SendHttpRequest::%d\n", nRet);
	printf("NetClient::Instance().SendHttpRequest::last\n");
	NetClient::Instance().ExitHttpClient();
	t1.join();

	return 0;
}