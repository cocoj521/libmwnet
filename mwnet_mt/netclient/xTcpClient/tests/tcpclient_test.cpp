#include "../TcpClient.h"
#include <mwnet_mt/util/MWEventLoop.h>
#include <boost/any.hpp>
#include <stdint.h>
#include <stdio.h>
#include <unistd.h>
#include <atomic>

#pragma GCC diagnostic ignored "-Wold-style-cast"
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wshadow"
#pragma GCC diagnostic ignored "-Wunused-variable"
#pragma GCC diagnostic ignored "-Wformat"


using namespace MWNET_MT::TCPCLIENT;
using namespace MWEVENTLOOP;

std::shared_ptr<mTcpClient> client;
char szMsg[128];
int func_on_connection(void* pInvoker, const ConnBaseInfoPtr& pConnBaseInfo, bool bConnected, const char* szIpPort)
{
	printf("totalconn:%ld,conn_uuid:%ld,bConn:%d,ConnStatus:%d,TimeConsume:%ldms,ConnStatusDesc:%s\n",
		client->GetTotalConnCnt(),
		pConnBaseInfo->conn_uuid, bConnected, pConnBaseInfo->conn_status, 
		pConnBaseInfo->conn_end_tm_ms-pConnBaseInfo->conn_begin_tm_ms,
		pConnBaseInfo->conn_status_desc.c_str());
	if (bConnected)
	{
		client->SendMsgWithTcpClient(pConnBaseInfo, bConnected, "POST", 4, 3);
	}
	else
	{
		client->DelConnection(pConnBaseInfo);

		ConnBaseInfoPtr p(new connection_baseinfo_t());
		p->conn_ip = pConnBaseInfo->conn_ip;
		p->conn_port = pConnBaseInfo->conn_port;
		p->conn_timeout = pConnBaseInfo->conn_timeout;
		p->any_data = pConnBaseInfo->any_data;

		client->AddConnection(p);
	}
	return 0;
}

int func_on_readmsg_tcp(void* pInvoker, const ConnBaseInfoPtr& pConnBaseInfo, const char* szMsg, int nMsgLen, int& nReadedLen)
{
	client->DelConnection(pConnBaseInfo);

	return 0;
}

int func_on_sendok(void* pInvoker, const ConnBaseInfoPtr& pConnBaseInfo, const boost::any& params)
{
	//printf("conn_uuid:%ld sendok\n",pConnBaseInfo->conn_uuid);
	return 0;
}

int func_on_senderr(void* pInvoker, const ConnBaseInfoPtr& pConnBaseInfo, const boost::any& params, int errCode)
{
	client->DelConnection(pConnBaseInfo);
	//printf("conn_uuid:%ld senderr\n", pConnBaseInfo->conn_uuid);
	return 0;
}

int func_on_highwatermark(void* pInvoker, const ConnBaseInfoPtr& pConnBaseInfo, size_t highWaterMark)
{
	//printf("conn_uuid:%ld highwatermark\n", pConnBaseInfo->conn_uuid);
	return 0;
}

int main(int argc, char* argv[])
{
	client.reset(new mTcpClient());
	bool bRet = client->InitTcpClient(NULL,
		func_on_connection,
		func_on_readmsg_tcp,
		func_on_sendok,
		func_on_senderr,
		func_on_highwatermark,
		8
		);
	if (bRet)
	{
		printf("init ok\n");
		for (int i = 0; i < 1000; i++)
		{
			ConnBaseInfoPtr pConnInfo(new connection_baseinfo_t());
			//pConnInfo->conn_ip = "175.25.22.9";
			//pConnInfo->conn_port = 9003;
			pConnInfo->conn_ip = "192.169.0.231";
			pConnInfo->conn_port = 7890;
			pConnInfo->conn_timeout = 5;
			pConnInfo->any_data = 1;

			client->AddConnection(pConnInfo);
		}
	}
	while(1)
	{
		//sleep(60);
		//client->UnInitTcpClient();
		sleep(3);
		printf("totalconncnt:%ld\n", client->GetTotalConnCnt());
		//sleep(1000);
	}

	return 0;
}
