
#include <iostream>
#include <thread>
#include <chrono>
#include <boost/any.hpp>
#include "../NetClient.h"

using namespace MWNET_MT;
using namespace MWNET_MT::CLIENT;

uint64_t g_uid(0);
int my_pfunc_on_readmsg_tcpclient(void* pInvoker, uint64_t conn_uuid, const boost::any& conn, const char* szMsg, int nMsgLen, int& nReadedLen)
{
	std::cout << "my_pfunc_on_readmsg_tcpclient: " << std::endl
		<< "conn_uuid:" << conn_uuid << std::endl
		<< "szMsg:" << szMsg << std::endl
		<< "nMsgLen:" << nMsgLen << std::endl << std::endl;
	g_uid = conn_uuid;
	nReadedLen = nMsgLen;
	return 0;
}

int my_pfunc_on_connection(void* pInvoker, uint64_t conn_uuid, const boost::any& conn, bool bConnected, const char* szIpPort)
{
	std::cout << "my_pfunc_on_connection: " << std::endl
		<< "conn_uuid:" << conn_uuid << std::endl
		<< "bConnected:" << bConnected << std::endl
		<< "szIpPort:" << szIpPort << std::endl << std::endl;

	return 0;
}

int my_pfunc_on_sendok(void* pInvoker, const uint64_t conn_uuid, const boost::any& conn)
{
	std::cout << "my_pfunc_on_sendok: " << std::endl
		<< "conn_uuid:" << conn_uuid << std::endl << std::endl;

	return 0;
}

int my_pfunc_on_heartbeat(const uint64_t conn_uuid)
{
	std::cout << "my_pfunc_on_heartbeat: " << std::endl
		<< "conn_uuid:" << conn_uuid << std::endl << std::endl;

	static int cnt(0);
	std::string line = std::to_string(cnt++);
	NetClient::Instance().SendTcpRequest(conn_uuid, 0, nullptr, line.c_str(), static_cast<int>(line.size()));

	return 0;
}

int main(int argc, char* argv[])
{
	std::string ip = "192.169.0.231";
	uint16_t port = 11111;
	

	
	std::string invoker = "me_invoker ";
	//void* pInvoker = (void*)invoker.c_str();
	int nThreadNum = 1;			/*线程数*/
	int idleconntime = 10;		/*空连接踢空时间*/
	int nMaxTcpClient = 2;		/*最大并发数*/

	uint64_t cli_uuid(0);
	size_t wnd_size = 1000;

	size_t nDefRecvBuf = 4 * 1024 * 1024;		/*默认接收buf*/
	size_t nMaxRecvBuf = 4 * 1024 * 1024;		/*最大接收buf*/
	size_t nMaxSendQue = 512;

	bool retvalue = NetClient::Instance().InitTcpClient(NULL, nThreadNum, idleconntime, nMaxTcpClient,
		ip, port, my_pfunc_on_connection, my_pfunc_on_sendok, my_pfunc_on_readmsg_tcpclient,
		nDefRecvBuf, nMaxRecvBuf, nMaxSendQue,
		cli_uuid, wnd_size);
	(void)retvalue;

	std::cout << "cli_uuid:" << cli_uuid << std::endl;


	NetClient::Instance().SetHeartbeat(my_pfunc_on_heartbeat);
	for (int i = 0; i < 1; ++i)
	{
		auto thread_spr = std::make_shared<std::thread>([=]()
		{
			
			while (1)
			{
				std::string line;
				static int x = 0;
				line = std::to_string(x++);

//				auto uuid = g_uid;
//
// 				int ret = NetClient::Instance().SendTcpRequest(uuid, 0, nullptr, line.c_str(), static_cast<int>(line.size()));
// 				std::cout << "SendTcpRequest ret" << ret << std::endl;
				std::this_thread::sleep_for(std::chrono::seconds(1));
			}
			
		});
		thread_spr->detach();
	}

// 	while (1)
// 		std::this_thread::sleep_for(std::chrono::seconds(1));

	std::string line;
	while (std::getline(std::cin, line))
	{
		if (line == "quit")
		{
			break;
		}

		int ret = NetClient::Instance().SendTcpRequest(cli_uuid, 0, nullptr, line.c_str(), static_cast<int>(line.size()));
		std::cout << "SendTcpRequest ret" << ret << std::endl;

	}

	NetClient::Instance().ExitTcpClient(0);

}






// 
// #include <iostream>
// #include <thread>
// #include <chrono>
// 
// #include <mwnet_mt/net/NetTcpClient.h>
// 
// using namespace mwnet_mt;
// 
// 
// void def_rms_cb(const boost::any& conn,
// 	const boost::any& c,
// 	const void* pInvokePtr,
// 	const char* command,
// 	int ret,
// 	const char* data,
// 	size_t len,
// 	int& nReadedLen,
// 	int64_t cur_time)
// {
// 	std::cout << "def_rms_cb " << std::endl
// 		<< " pInvokePtr =" << static_cast<const char*>(pInvokePtr) << std::endl
// 		<< " command =" << command << std::endl
// 		<< " ret =" << ret << std::endl
// 		<< " data =" << data << std::endl
// 		<< " len =" << len << std::endl
// 		<< " nReadedLen =" << nReadedLen << std::endl
// 		<< " cur_time =" << cur_time << std::endl
// 		<< std::endl;
// 
// 	nReadedLen = static_cast<int>(len);
// }
// int main(int argc, char* argv[])
// {
// 	RmsInitialise("my_pInvoke", def_rms_cb, 4, 5, 30);
// 
// 	std::string ip = "127.0.0.1";
// 	uint16_t port = 11100;
// 	RmsStart(ip, port);
// 
// 
// 
// 	std::string line;
// 	while (std::getline(std::cin, line))
// 	{
// 		if (line == "quit")
// 		{
// 			break;
// 		}
// 
// 		int ret = RmsSend(line.c_str(), static_cast<int>(line.size()));
// 		std::cout << "RmsSend ret" << ret << std::endl;
// 
// 	}
// 
// 	RmsStop();
// 
// 	// 等待连接完美断掉，否则要自己处理抛出的异常
// 	std::this_thread::sleep_for(std::chrono::seconds(2));
// 	RmsUnInitialise();
// 
// }
// 
// 
// 
