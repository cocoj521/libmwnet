#include "../MWLogger.h"

using namespace MWLOGGER;

std::string getTime()
{
	time_t timep;
	time(&timep);
	char tmp[64];
	strftime(tmp, sizeof(tmp), "%Y-%m-%d %H:%M:%S", localtime(&timep));
	return tmp;
}

int main()
{
	const char* objLogp[] = {"jhbtest"};
	MWLOGGER::Logger::SetLogParams(0, 1, 10, 1, 7, 1);
	std::string strExecDir = "./";
	//MWLOGGER::Logger::GetExecDir(strExecDir);
	printf("%s\n", strExecDir.c_str());
	MWLOGGER::Logger::CreateLogObjs(strExecDir, "testmwlogger/", 1, objLogp, false);
	//int n = 0;
	while (1)
	{
		for (int i = 0; i < 1000; i++)
		{
			LOG_INFO(0,OUTPUT_TYPE::LOCAL_FILE) << getTime();
		}
		///LOG_INFO(0, OUTPUT_TYPE::PRINT_STDOUT) << getTime();
		sleep(1);
	}

	return 0;
}