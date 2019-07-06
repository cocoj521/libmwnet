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


using namespace MWNET_MT::CLIENT;
using namespace MWEVENTLOOP;

MWEVENTLOOP::EventLoop eventloop1_;
MWEVENTLOOP::EventLoop eventloop2_;
MWEVENTLOOP::EventLoop eventloop3_;
MWEVENTLOOP::EventLoop eventloop4_;
MWEVENTLOOP::EventLoop eventloop5_;

MWNET_MT::CLIENT::NetClient client_;
std::atomic<int> g_total_send;
std::atomic<int> g_total_recv;

int func_on_connection(void* pInvoker, uint64_t conn_uuid, bool bConnected, const char* szIpPort, const char* szIp, uint16_t port)
{
	if (bConnected)
	{
		printf("%ld-%s-UP\n", conn_uuid, szIpPort);
		//client_.SendTcpRequest(nullptr,"123",3,true);
		//client_.DisConnect();
		//usleep(10*1000);
		client_.SendTcpRequest(nullptr, "hello", sizeof("hello"), true);
	}
	else
	{
		if (conn_uuid > 0)
		{
			printf("%ld-%s-DWON\n", conn_uuid, szIpPort);
			//client_.ReConnect("192.169.0.231", 1234);
			//client_.Connect();
		}
		else
		{
			printf("%s-CONNECT FAIL\n", szIpPort);
		}
		client_.ReConnect("192.169.0.231", 1234);
	}
	return 0;
}

// 网络收到数据回调函数
//返回值0:代表成功 >0代表其他各种错误或反控等信息(待定) <0:底层会主动强制关闭连接
int func_on_readmsg_tcpclient(void* pInvoker, uint64_t conn_uuid, const char* szMsg, int nMsgLen, int& nReadedLen)
{
	//printf("RECV:%s\n", szMsg);
	nReadedLen = nMsgLen;
	++g_total_recv;
	client_.SendTcpRequest(nullptr, "hello", sizeof("hello"), true);
	return 0;
}

// 网络数据发送完成回调函数
//返回值0:代表成功 >0代表其他各种错误或反控等信息(待定) <0:底层会主动强制关闭连接
int func_on_sendok(void* pInvoker, uint64_t conn_uuid, const boost::any& params)
{
	++g_total_send;
	
	//client_.SendTcpRequest(nullptr,"456",3,true);

	return 0;
}

//高水位回调 当底层缓存的待发包数量超过设置的值时会触发该回调
//返回值0:代表成功 >0代表其他各种错误或反控等信息(待定) <0:底层会主动强制关闭连接
int func_on_highwatermark(void* pInvoker, const uint64_t conn_uuid, size_t highWaterMark)
{
	return 0;
}

void do_reconnect1()
{
	client_.ReConnect("192.169.0.231", 1235);
}

void do_reconnect2()
{
	client_.ReConnect("192.169.0.231", 1336);
}

int main(int argc, char* argv[])
{
	client_.InitTcpClient(NULL, "192.169.0.231", 1234, func_on_connection, func_on_readmsg_tcpclient, func_on_sendok, func_on_highwatermark);
	// sleep(2);
	// client_.ReConnect("192.169.0.231", 1235);

	//eventloop1_.RunEvery(0.1, do_reconnect1);
	//eventloop2_.RunEvery(0.1, do_reconnect1);
	//eventloop3_.RunEvery(0.1, do_reconnect2);
	//eventloop4_.RunEvery(0.1, do_reconnect1);
	//eventloop5_.RunEvery(0.1, do_reconnect1);
	

	while(1)
	{
		//client_.DisConnect();
		//client_.Connect();
		//client_.ReConnect("192.169.0.231", 1235);
		//usleep(500);
		static int last_send = 0;
		sleep(1);
		printf("send:%d\nrecv:%d\nspd:%d\n",g_total_send.load(),g_total_recv.load(),g_total_send.load()-last_send);
		last_send=g_total_send.load();
	}
}
