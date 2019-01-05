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
	const char* objLogp[] = { "zbs"};
	MWLOGGER::Logger::SetLogParams();
	MWLOGGER::Logger::CreateLogObjs("", "zbs", 1, objLogp, false);
	int n = 0;
	while (1)
	{
		for (int i = 0; i < 1; i++)
		{
			LOG_INFO(i,OUTPUT_TYPE::LOCAL_FILE) << getTime();
		}
		LOG_INFO(0, OUTPUT_TYPE::PRINT_STDOUT).Format(">>%s-[%d]", getTime().c_str(), n++);
		sleep(1);
	}

	return 0;
}