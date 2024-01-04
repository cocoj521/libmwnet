#include <mwnet_mt/util/MWEventLoop.h>
#include <mwnet_mt/util/MWThread.h>
#include <mwnet_mt/util/MWThreadPool.h>
#include <mwnet_mt/util/MWSafeLock.h>
#include <mwnet_mt/util/MWAtomicInt.h>
#include <mwnet_mt/logger/MWLogger.h>
#include <mwnet_mt/util/MWStringUtil.h>
#include <mwnet_mt/util/MWTimestamp.h>

#include <stdint.h>
#include <pthread.h>
#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>

#include <boost/any.hpp>
#include <sys/prctl.h>
#include <sys/syscall.h>
#include <arpa/inet.h>
#include <stdarg.h>
#include <algorithm>
#include <sys/time.h>
#include <sys/stat.h>
#include <unistd.h>
#include <cmath>
#include <map>

#pragma GCC diagnostic ignored "-Wold-style-cast"
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wshadow"
#pragma GCC diagnostic ignored "-Wunused-variable"
#pragma GCC diagnostic ignored "-Wformat"

using namespace MWEVENTLOOP;
using namespace MWSAFELOCK;
using namespace MWATOMICINT;
using namespace MWLOGGER;
using namespace MWSTRINGUTIL;
using namespace MWTIMESTAMP;

size_t get_executable_path(char* processdir, char* processname, size_t len)
{
    char* path_end = NULL;
    if(::readlink("/proc/self/exe", processdir,len) <=0) return -1;
    path_end = strrchr(processdir,  '/');
    if(path_end == NULL)  return -1;
    ++path_end;
    strcpy(processname, path_end);
    *path_end = '\0';
    return (size_t)(path_end - processdir);
}

void MyPrint1()
{
	printf("myprint1....\n");
	sleep(10);
}

void MyPrint2()
{
	printf("myprint2....\n");
}

void MyPrint3()
{
	printf("myprint3....\n");
}
void MyPrint4(int n)
{
	printf("myprint4....%d\n", n);
}

void DealTimeOut(const boost::any& any_params)
{
	printf("deal timeout data....%d\n", boost::any_cast<int>(any_params));
}
//业务模块日志枚举,将其用作ID,用来标识一个日志
enum MODULE_LOGS
{
	MODULE_LOGS_DEFAULT,	//默认模块,如主模块
	MODULE_LOGS_DOWNLOAD,	//各种不同的模块.....有多少个想输出日志的模块,就定义多少个,每定义一个,就会生成一个日志
	MODULE_LOG_NUMS,		//日志个数
};			
//定义不同模块的日志名称,必须与MODULE_LOGS中除MODULE_LOG_NUMS外一一对应!!!
//每次增加一个日志时g_module_logs和MODULE_LOGS都要对应的加进相应的值
const char* g_module_logs[] = 
{
"syslog",
"download"
};

int main(int argc, char* argv[])
{
	LOG_INIT_PARAMS(0,1,100,3,7,5);
	LOG_CREATE("./", "testmwlogger/", MODULE_LOG_NUMS, g_module_logs, false);
	LOG_SET_LEVEL(MWLOGGER::INFO);
	LOG_INFO(MODULE_LOGS_DEFAULT,MWLOGGER::LOCAL_FILE)<<"test111test111test111test111test111test111test111";
	LOG_INFO(MODULE_LOGS_DOWNLOAD,MWLOGGER::LOCAL_FILE)<<"test222test222test222test222test222test222test222";
	LOG_INFO(MODULE_LOGS_DEFAULT,MWLOGGER::LOCAL_FILE).Format("----%s--%d", "test", 123);
	
	char path[256] = {0};
    char processname[256] = {0};
    get_executable_path(path, processname, sizeof(path));
		
	::prctl(PR_SET_NAME, processname);

	MWATOMICINT::AtomicInt32  test_int32;
	test_int32.Inc();
	printf("%d\n",test_int32.Get());

	{
		MWSAFELOCK::BaseLock lock;
		MWSAFELOCK::SafeLock safelock(&lock);
	}
	/*
	
	*/

	std::string strtohex = "00123^^&&%!%$";
	std::string strHex = "";
	printf(MWSTRINGUTIL::StringUtil::BytesToHexString(strtohex, strHex).c_str());
	printf("\n");

	printf(MWSTRINGUTIL::StringUtil::BytesToHexString(strtohex, strHex, true).c_str());
	printf("\n");

	std::string strBin = "";
	printf(MWSTRINGUTIL::StringUtil::HexStringToBytes(strHex.data(), strHex.size(), strBin).c_str());
	printf("\n");

	std::string base64Str = "";

	printf(MWSTRINGUTIL::StringUtil::BytesToBase64String(strtohex.data(), strtohex.size(), base64Str).c_str());	
	printf("\n");

	printf(MWSTRINGUTIL::StringUtil::Base64StringToBytes(base64Str.data(), base64Str.size(), strHex).c_str());	
	printf("\n");

	std::string md5HexStr = "";
	printf(MWSTRINGUTIL::StringUtil::ToMd5HexString(strtohex.data(), strtohex.size(), md5HexStr).c_str());	
	printf("\n");

	std::cout << MWSTRINGUTIL::StringUtil::ToString(3999999.1415926);
	printf("\n");
	std::cout << std::to_string(3999999.1415926);
	printf("\n");

	std::string strFormat = "";
	printf(MWSTRINGUTIL::StringUtil::FormatString(strFormat, "%d",123456).c_str());	
	printf("\n");
	

	printf("begin-100W sha1:%s\n", MWTIMESTAMP::Timestamp::Now().ToFormattedString().c_str());
	for (int i = 0; i <= 10000000; ++i)
	{
		std::string sha1HexStr;
		MWSTRINGUTIL::StringUtil::ToSha1HexString("8613265661403", sha1HexStr);
	}
	printf("end-100W sha1:%s\n", MWTIMESTAMP::Timestamp::Now().ToFormattedString().c_str());

	while(1)
	{
		sleep(1);
	}
}

/*
int main(int argc, char* argv[])
{
	char path[256] = {0};
    char processname[256] = {0};
    get_executable_path(path, processname, sizeof(path));
		
	::prctl(PR_SET_NAME, processname);

	{
		MWTHREAD::Thread thr(MyPrint1, "test");
		
		pid_t pd = thr.StartThread();

		int ret = -1;
		
		printf("test thread:%d,%d\n", pd, ret);

		//ret = thr.JoinThread();
		
		printf("test thread:%d,%d\n", pd, ret);
	}


	{
		MWTHREAD::Thread thr(std::bind(MyPrint4, 10), "test");
		
		pid_t pd = thr.StartThread();

		int ret = -1;
		
		printf("test thread:%d,%d\n", pd, ret);

		ret = thr.JoinThread();
		
		printf("test thread:%d,%d\n", pd, ret);
	}

	//{
		std::unique_ptr<MWTHREAD::Thread> pThr(new MWTHREAD::Thread(std::bind(MyPrint4, 10), "test"));
	
		pid_t pd = pThr->StartThread();

		int ret = -1;
		
		printf("test thread:%d,%d\n", pd, ret);

		//ret = pThr->JoinThread();
		
		printf("test thread:%d,%d\n", pd, ret);
	//}
		
	while(1)
	{
		sleep(1);
	}
}
*/
/*
int main(int argc, char* argv[])
{
	char path[256] = {0};
    char processname[256] = {0};
    get_executable_path(path, processname, sizeof(path));
		
	::prctl(PR_SET_NAME, processname);

	{
		MWTHREADPOOL::ThreadPool thrpool("test");
		
		thrpool.SetMaxTaskQueueSize(100);
		thrpool.SetThreadInitCallback(MyPrint1);
		thrpool.StartThreadPool(2);
		thrpool.TryRunTask(MyPrint2);
		thrpool.StopThreadPool();
	}


	{
		MWTHREADPOOL::ThreadPool thrpool("test");
		
		thrpool.SetMaxTaskQueueSize(100);
		thrpool.SetThreadInitCallback(std::bind(MyPrint4,400));
		thrpool.StartThreadPool(2);
		thrpool.TryRunTask(MyPrint1);
		thrpool.TryRunTask(std::bind(MyPrint4,400));
		thrpool.StopThreadPool();
	}

	//{
		std::unique_ptr<MWTHREADPOOL::ThreadPool> pThrPool(new MWTHREADPOOL::ThreadPool("test"));
	
		pThrPool->SetMaxTaskQueueSize(100);
		pThrPool->SetThreadInitCallback(std::bind(MyPrint4,400));
		pThrPool->StartThreadPool(2);
		pThrPool->TryRunTask(MyPrint3);
		pThrPool->TryRunTask(std::bind(MyPrint4,400));
		printf("%s\n", pThrPool->TheadPoolName());
		pThrPool->StopThreadPool();
	//}
		
	while(1)
	{
		sleep(1);
	}
}
*/

