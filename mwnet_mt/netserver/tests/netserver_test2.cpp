#include "../NetServer.h"
#include <thread>
#include <stdio.h>

using namespace MWNET_MT::SERVER;

NetServer HttpSvr_8888;
NetServer HttpSvr_9999;


int onHttpConnection(void* pInvoker, uint64_t connUuid, const boost::any& conn, bool bConnected, const char* szIpPort)
{
	printf("connUuid: %lu, szIpPort: %s %s", connUuid, szIpPort, (bConnected ? " UP" : " DOWN"));
	return 0;
}

int onHttpMessage_8888(void* pInvoker, uint64_t connUuid, const boost::any& conn, const MWNET_MT::SERVER::HttpRequest* request)
{
	if (!pInvoker || !request)
	{
		//return -1;
	}

	std::string ipAddr = request->GetClientRequestIp();
	//if (requestIpLocalArea(ipAddr))
	{
		if (request->GetRequestType() == MWNET_MT::SERVER::HTTP_REQUEST_TYPE::HTTP_REQUEST_POST)
		{
			std::string infoUrl;
			size_t len = 0;
			len = request->GetUrl(infoUrl);
			printf("HTTP request post url len :: %ld %s", len, infoUrl.c_str());
		}
	}
	MWNET_MT::SERVER::HttpResponse rsp;
	rsp.SetKeepAlive(false);
	rsp.SetStatusCode(200);
	rsp.SetContentType("XXXXXXXX");
	rsp.SetResponseBody("8888 RESPONESE");
	int ret = 1;
	HttpSvr_8888.SendHttpResponse(connUuid, conn, ret, rsp);
	return 0;
}
int onHttpMessage_9999(void* pInvoker, uint64_t connUuid, const boost::any& conn, const MWNET_MT::SERVER::HttpRequest* request)
{
	if (!pInvoker || !request)
	{
		return -1;
	}

	std::string ipAddr = request->GetClientRequestIp();
	//if (requestIpLocalArea(ipAddr))
	{
		if (request->GetRequestType() == MWNET_MT::SERVER::HTTP_REQUEST_TYPE::HTTP_REQUEST_POST)
		{
			std::string infoUrl;
			size_t len = 0;
			len = request->GetUrl(infoUrl);
			printf("HTTP request post url len :: %ld %s", len, infoUrl.c_str());
		}
	}
	MWNET_MT::SERVER::HttpResponse rsp;
	rsp.SetKeepAlive(false);
	rsp.SetStatusCode(200);
	rsp.SetContentType("XXXXXXXX");
	rsp.SetResponseBody("9999 RESPONESE");
	int ret = 1;
	HttpSvr_9999.SendHttpResponse(connUuid, conn, ret, rsp);
	return 0;
}


int a = 0;

int onHttpSendComplete(void* pInvoker, uint64_t connUuid, const boost::any& conn, const boost::any& params)
{
	printf("connUuid :: %lu send ok", connUuid);
	return 0;
}

void Init_8888()
{
	HttpSvr_8888.StartHttpServer(&a, 8888, 1, 20, 100, true, &onHttpConnection, &onHttpMessage_8888, &onHttpSendComplete);
}

void Init_9999()
{
	HttpSvr_9999.StartHttpServer(&a, 9999, 1, 20, 100, true, &onHttpConnection, &onHttpMessage_9999, &onHttpSendComplete);
}

int main()
{
	printf("main start\n");
	std::thread t8(Init_8888);
	std::thread t9(Init_9999);

	while (1);
}