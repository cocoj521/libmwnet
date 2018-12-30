#include <iostream>
#include <thread>
#include <chrono>

#include "../TcpClient.h"

using namespace MWNET_MT;
using namespace MWNET_MT::CLIENT;


void def_recv_cb(const void* pInvokePtr,
	int nNodeNum,			// 节点
	int command,	// 0 on_conn连接、1 on_send发送、2 on_recv接收
	int ret,				// true成功, 其他失败
	const char* data,
	size_t len,
	int& nReadedLen)
{
	std::string strcmd;
	switch (command)
	{
	case 0:
		strcmd = "on_conn";
		break;
	case 1:
		strcmd = "on_send";
		break;
	case 2:
		strcmd = "on_recv";
		break;
	default:
		break;
	}
	std::cout << "def_recv_cb " << std::endl
		<< " pInvokePtr =" << static_cast<const char*>(pInvokePtr) << std::endl
		<< " nNodeNum =" << nNodeNum << std::endl
		<< " command =" << strcmd << std::endl
		<< " ret =" << ret << std::endl
		<< " data =" << data << std::endl
		<< " len =" << len << std::endl
		<< " nReadedLen =" << nReadedLen << std::endl
		<< std::endl;

	nReadedLen = static_cast<int>(len);
}
int main(int argc, char* argv[])
{
	Params params;
	params.bAutoRecon = true;
	params.nThreads = 1;
	params.nWnd = 1000;

	params.nMaxBuf = 4 * 1024 * 1024;
	params.nAutoReconTime = 10;
	params.bMayUseBlockOpration = true;

	XTcpClient xcli;
	xcli.Init("my_pInvoke", params, def_recv_cb);
	
	int g_mode = 1;
	printf("input mode(syn=1, asyn=0):\n");
	scanf("%d", &g_mode);
	printf("mode:%d\n", g_mode);


	std::vector<int> vec_id;
	int cli_cnt = 3;
	for (int i = 0; i < cli_cnt; ++i)
	{
		std::string ip = "192.169.0.60";
		int port = 11100;
		int moder = g_mode; // 0 异步， 1 同步
		int ms_timeout = 3000;
		int nLinkId = xcli.ApplyNewLink(ip, port, moder, ms_timeout);
		std::cout << "ApplyNewLink ret:" << nLinkId
			<< " ip:" << ip
			<< " port:" << port
			<< " mode:" << moder
			<< " timout:" << ms_timeout
			<< std::endl;

		vec_id.emplace_back(nLinkId);
	}

	std::string line;
	while (std::getline(std::cin, line))
	{
		if (line == "quit")
		{
			break;
		}

		// 测试 CloseLink
		if (line == "close" && vec_id.size() >= 2)
		{
			int nLinkId = vec_id[0];
			vec_id.erase(vec_id.begin());
			int ret = xcli.CloseLink(nLinkId);
			std::cout << "CloseLink nLinkId:" << ret << std::endl;

			continue;
		}

		if (line == "retry")
		{
			int nLinkId = vec_id[0];
			int ret = xcli.ReConnLink(nLinkId);
			std::cout << "ReConnLink nLinkId:" << nLinkId 
				<< " ret:" << ret << std::endl;

			continue;
		}

		// 获取nLinkId
		static int x = 0;
		int nLinkId = vec_id[static_cast<int>(x++ % vec_id.size())];

		if (line == "info")
		{
			std::cout << "LinkState:" << xcli.LinkState(nLinkId) << std::endl;
			std::cout << "GetWaitBufSize:" << xcli.GetWaitBufSize(nLinkId) << std::endl;
			std::cout << "GetAllWaitBufSize:" << xcli.GetAllWaitBufSize() << std::endl;
			std::cout << "GetUseWnd:" << xcli.GetUseWnd(nLinkId) << std::endl;
			continue;
		}

		int ret = xcli.Send(nLinkId, line.c_str(), static_cast<int>(line.size()));
		std::cout << "Send ret:" << ret
			<< " nLinkId:" << nLinkId
			<< std::endl;

	}

	for (auto nLinkId : vec_id)
		xcli.CloseLink(nLinkId);

	std::this_thread::sleep_for(std::chrono::seconds(2));
	xcli.UnInit();

}


/*
void def_rms_cb(const void* pInvokePtr,
	int nNodeNum,			// 节点
	const char* command,	// on_conn连接、on_send发送、on_recv接收
	int ret,				// true成功, 其他失败
	const char* data,
	size_t len,
	int& nReadedLen,
	int64_t cur_time)
{
	std::cout << "def_rms_cb " << std::endl
		<< " pInvokePtr =" << static_cast<const char*>(pInvokePtr) << std::endl
		<< " nNodeNum =" << nNodeNum << std::endl
		<< " command =" << command << std::endl
		<< " ret =" << ret << std::endl
		<< " data =" << data << std::endl
		<< " len =" << len << std::endl
		<< " nReadedLen =" << nReadedLen << std::endl
		<< " cur_time =" << cur_time << std::endl
		<< std::endl;

	nReadedLen = static_cast<int>(len);
}
int main(int argc, char* argv[])
{
	RMS_PARAMS rms_param;
	rms_param.iClientsPerNode = 4;

	{
		RMS_LINKNODE node1;
		node1.nNodeNum = 555;
		strcpy(node1.szIp, "127.0.0.1");
		node1.nPort = 11100;
		rms_param.vecLinkNode.push_back(node1);
	}

	{
		RMS_LINKNODE node1;
		node1.nNodeNum = 556;
		strcpy(node1.szIp, "127.0.0.1");
		node1.nPort = 22200;
		rms_param.vecLinkNode.push_back(node1);
	}

	RmsInitialise("my_pInvoke", def_rms_cb, rms_param, 5, 30);


	std::string line;
	while (std::getline(std::cin, line))
	{
		if (line == "quit")
		{
			break;
		}

		int index = 555;
		static int x = 0;
		x++;
		if (x % 2)
			index = 555;
		else
			index = 556;
		int ret = RmsSend(index, line.c_str(), static_cast<int>(line.size()));
		std::cout << "RmsSend ret" << ret << std::endl;

	}

	RmsStop();

	// 等待连接完美断掉，否则要自己处理抛出的异常
	std::this_thread::sleep_for(std::chrono::seconds(2));
	RmsUnInitialise();

}
*/



