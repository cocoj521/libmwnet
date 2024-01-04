#ifndef MW_LOGGER_H
#define MW_LOGGER_H

#include <boost/any.hpp>
#include <string>
#include <memory>
#include <thread>

namespace MWLOGGER
{
typedef std::shared_ptr<std::string> LogStringPtr;

namespace MWLOGSTREAM
{
class LogStream
{
public:
	LogStream();
public:
	LogStream& operator<<(bool v);
	LogStream& operator<<(short);
	LogStream& operator<<(unsigned short);
	LogStream& operator<<(int);
	LogStream& operator<<(unsigned int);
	LogStream& operator<<(long);
	LogStream& operator<<(unsigned long);
	LogStream& operator<<(long long);
	LogStream& operator<<(unsigned long long);
	LogStream& operator<<(const void*);
	LogStream& operator<<(float v); 
	LogStream& operator<<(double);
	LogStream& operator<<(char v);
	LogStream& operator<<(const char* str);
	LogStream& operator<<(const unsigned char* str);
	LogStream& operator<<(const std::string& v);
public:
	void append(const char* data, size_t len);
	const LogStringPtr& getbuffer() const;
	void resetbuffer();
	LogStream& Format(const char * _Format, ...);
private:
	template<typename T>
	void formatInteger(T);
private:
	LogStringPtr m_LogStrPtr;
};
}

/////////////////////////////////////////////////////////////////////////////////////////
//日志级别
enum LOG_LEVEL
{
	TRACE,
	DEBUG,
	INFO,
	WARN,
	ERROR,
	FATAL,
	NUM_LOG_LEVELS,
};

//日志输出的方式
enum OUTPUT_TYPE
{
	LOCAL_FILE,		//输出到本地文件
	PRINT_STDOUT,	//直接打印
	OUT_ALL,		//输出全部
};

class Logger
{
public:
	//x:日志文件编号,optype:输出方式(打印/写文件,...),level:日志级别,file,line,func:源文件,行号,函数名
	Logger(int x, OUTPUT_TYPE optype, LOG_LEVEL level, const char* file, int line, const char* func);
	~Logger();
public:	
	MWLOGSTREAM::LogStream& stream();
public:
	//step 1.......
	//flush_freq,flush频率即每隔多久把日志刷入输出介质,单位:秒,默认值:3, 
	//flush_every,flush条数即每多少条日志才刷入输出介质),单位:条,默认值:1000
	//roll_size,日志滚动的大小即每个日志文件的最大大小,单位:M,默认值100, 
	//zip_day,日志压缩天数即压缩N天前的日志,单位:天,默认值3
	//save_day,日志保存天数即超过N天前的日志会被删除,单位:天,默认值7
	//zip_minute,日志压缩分钟数即已切换的旧日志N分钟后会被压缩,单位:分,默认值5,>0时表示启动压缩
	static void SetLogParams(int flush_freq=3, int flush_every=1000, int roll_size=100, int zip_day=3, int save_day=7, int zip_minute=5);

	//step 2......
	//创建一定数量的日志对象,传入不同的日志对象编号,以及日志对象名称
	//basepath:日志路径(尽量填绝对路径,否则日志自动压缩备份功能会受影响,填""、"."、"./"内部分自动取程序运行的路径) 如: /home/aa/bb/
	//basedir:日志主目录 如: netlogs/
	//obj_cnt:日志路径下的主目录内创建多少个子日志 
	//asynclog:是否异步记日志(异步会启线程记日志)
	//目录结构 日志路径+日志主目录+子日志名称+YYMMDD+子日志名称-hhmmss.log
	//如:/home/jhb/+logs+downloadlog+20180101+downloadlog-120530.log
	static bool CreateLogObjs(const std::string& basepath, const std::string& basedir, int obj_cnt, const char* logs[], bool asynclog=false);

	//step 3.........
	//设置所有日志对象同一个日志级别
	static void SetLogLevel(LOG_LEVEL level);

	//step 3.........
	//根据日志编号设置该日志的日志级别
	static void SetLogLevel(int x/*日志对象编号*/, LOG_LEVEL level);

	//根据日志编号获取该日志的日志级别
	static LOG_LEVEL GetLogLevel(int x/*日志对象编号*/);
	
	//获取程序运行目录,如:/home/aa/bb/
	static bool GetExecDir(std::string& strExecDir);

	//日志模块退出
	static void Exit();
private:
	boost::any m_any;
	//输出方式
	OUTPUT_TYPE m_optype;
	//日志文件编号
	int m_x;
};

//日志初始化参数
#define LOG_INIT_PARAMS(p1,p2,p3,p4,p5,p6) MWLOGGER::Logger::SetLogParams(p1,p2,p3,p4,p5,p6)

//日志模块退出
#define LOG_EXIT() MWLOGGER::Logger::Exit()

//日志创建
#define LOG_CREATE(p1,p2,p3,p4,p5) MWLOGGER::Logger::CreateLogObjs(p1,p2,p3,p4,false)

//设置日志级别
#define LOG_SET_LEVEL(x)   		 MWLOGGER::Logger::SetLogLevel(x)
#define LOG_SET_LEVEL_INDEX(x,y) MWLOGGER::Logger::SetLogLevel(x,y)


//x:日志编号  y:日志输出的方式(打印/文件,...)
#define LOG_TRACE(x,y) if (MWLOGGER::Logger::GetLogLevel(x) <= MWLOGGER::TRACE) MWLOGGER::Logger(x, y, MWLOGGER::TRACE, __FILE__, __LINE__, __func__).stream()
#define LOG_DEBUG(x,y) if (MWLOGGER::Logger::GetLogLevel(x) <= MWLOGGER::DEBUG) MWLOGGER::Logger(x, y, MWLOGGER::DEBUG, __FILE__, __LINE__, __func__).stream()
#define LOG_INFO(x,y)  if (MWLOGGER::Logger::GetLogLevel(x) <= MWLOGGER::INFO ) MWLOGGER::Logger(x, y, MWLOGGER::INFO , __FILE__, __LINE__, __func__).stream()
#define LOG_WARN(x,y)  if (MWLOGGER::Logger::GetLogLevel(x) <= MWLOGGER::WARN ) MWLOGGER::Logger(x, y, MWLOGGER::WARN , __FILE__, __LINE__, __func__).stream()
#define LOG_ERROR(x,y) if (MWLOGGER::Logger::GetLogLevel(x) <= MWLOGGER::ERROR) MWLOGGER::Logger(x, y, MWLOGGER::ERROR, __FILE__, __LINE__, __func__).stream()
#define LOG_FATAL(x,y) if (MWLOGGER::Logger::GetLogLevel(x) <= MWLOGGER::FATAL) MWLOGGER::Logger(x, y, MWLOGGER::FATAL, __FILE__, __LINE__, __func__).stream()

}
#endif  

// 用法栗子
/*
using namespace MWLOGGER;

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

int main()
{
	LOG_INIT_PARAMS(0,1,100,3,7,5);
	LOG_CREATE("./", "testmwlogger/", MODULE_LOG_NUMS, g_module_logs, false);
	LOG_SET_LEVEL(MWLOGGER::INFO);
	LOG_INFO(MODULE_LOGS_DEFAULT,MWLOGGER::LOCAL_FILE)<<"test111test111test111test111test111test111test111";
	LOG_INFO(MODULE_LOGS_DOWNLOAD,MWLOGGER::LOCAL_FILE)<<"test222test222test222test222test222test222test222";
	LOG_INFO(MODULE_LOGS_DEFAULT,MWLOGGER::LOCAL_FILE).Format("----%s--%d-", "test", 123);
}
*/
