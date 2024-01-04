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
//��־����
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

//��־����ķ�ʽ
enum OUTPUT_TYPE
{
	LOCAL_FILE,		//����������ļ�
	PRINT_STDOUT,	//ֱ�Ӵ�ӡ
	OUT_ALL,		//���ȫ��
};

class Logger
{
public:
	//x:��־�ļ����,optype:�����ʽ(��ӡ/д�ļ�,...),level:��־����,file,line,func:Դ�ļ�,�к�,������
	Logger(int x, OUTPUT_TYPE optype, LOG_LEVEL level, const char* file, int line, const char* func);
	~Logger();
public:	
	MWLOGSTREAM::LogStream& stream();
public:
	//step 1.......
	//flush_freq,flushƵ�ʼ�ÿ����ð���־ˢ���������,��λ:��,Ĭ��ֵ:3, 
	//flush_every,flush������ÿ��������־��ˢ���������),��λ:��,Ĭ��ֵ:1000
	//roll_size,��־�����Ĵ�С��ÿ����־�ļ�������С,��λ:M,Ĭ��ֵ100, 
	//zip_day,��־ѹ��������ѹ��N��ǰ����־,��λ:��,Ĭ��ֵ3
	//save_day,��־��������������N��ǰ����־�ᱻɾ��,��λ:��,Ĭ��ֵ7
	//zip_minute,��־ѹ�������������л��ľ���־N���Ӻ�ᱻѹ��,��λ:��,Ĭ��ֵ5,>0ʱ��ʾ����ѹ��
	static void SetLogParams(int flush_freq=3, int flush_every=1000, int roll_size=100, int zip_day=3, int save_day=7, int zip_minute=5);

	//step 2......
	//����һ����������־����,���벻ͬ����־������,�Լ���־��������
	//basepath:��־·��(���������·��,������־�Զ�ѹ�����ݹ��ܻ���Ӱ��,��""��"."��"./"�ڲ����Զ�ȡ�������е�·��) ��: /home/aa/bb/
	//basedir:��־��Ŀ¼ ��: netlogs/
	//obj_cnt:��־·���µ���Ŀ¼�ڴ������ٸ�����־ 
	//asynclog:�Ƿ��첽����־(�첽�����̼߳���־)
	//Ŀ¼�ṹ ��־·��+��־��Ŀ¼+����־����+YYMMDD+����־����-hhmmss.log
	//��:/home/jhb/+logs+downloadlog+20180101+downloadlog-120530.log
	static bool CreateLogObjs(const std::string& basepath, const std::string& basedir, int obj_cnt, const char* logs[], bool asynclog=false);

	//step 3.........
	//����������־����ͬһ����־����
	static void SetLogLevel(LOG_LEVEL level);

	//step 3.........
	//������־������ø���־����־����
	static void SetLogLevel(int x/*��־������*/, LOG_LEVEL level);

	//������־��Ż�ȡ����־����־����
	static LOG_LEVEL GetLogLevel(int x/*��־������*/);
	
	//��ȡ��������Ŀ¼,��:/home/aa/bb/
	static bool GetExecDir(std::string& strExecDir);

	//��־ģ���˳�
	static void Exit();
private:
	boost::any m_any;
	//�����ʽ
	OUTPUT_TYPE m_optype;
	//��־�ļ����
	int m_x;
};

//��־��ʼ������
#define LOG_INIT_PARAMS(p1,p2,p3,p4,p5,p6) MWLOGGER::Logger::SetLogParams(p1,p2,p3,p4,p5,p6)

//��־ģ���˳�
#define LOG_EXIT() MWLOGGER::Logger::Exit()

//��־����
#define LOG_CREATE(p1,p2,p3,p4,p5) MWLOGGER::Logger::CreateLogObjs(p1,p2,p3,p4,false)

//������־����
#define LOG_SET_LEVEL(x)   		 MWLOGGER::Logger::SetLogLevel(x)
#define LOG_SET_LEVEL_INDEX(x,y) MWLOGGER::Logger::SetLogLevel(x,y)


//x:��־���  y:��־����ķ�ʽ(��ӡ/�ļ�,...)
#define LOG_TRACE(x,y) if (MWLOGGER::Logger::GetLogLevel(x) <= MWLOGGER::TRACE) MWLOGGER::Logger(x, y, MWLOGGER::TRACE, __FILE__, __LINE__, __func__).stream()
#define LOG_DEBUG(x,y) if (MWLOGGER::Logger::GetLogLevel(x) <= MWLOGGER::DEBUG) MWLOGGER::Logger(x, y, MWLOGGER::DEBUG, __FILE__, __LINE__, __func__).stream()
#define LOG_INFO(x,y)  if (MWLOGGER::Logger::GetLogLevel(x) <= MWLOGGER::INFO ) MWLOGGER::Logger(x, y, MWLOGGER::INFO , __FILE__, __LINE__, __func__).stream()
#define LOG_WARN(x,y)  if (MWLOGGER::Logger::GetLogLevel(x) <= MWLOGGER::WARN ) MWLOGGER::Logger(x, y, MWLOGGER::WARN , __FILE__, __LINE__, __func__).stream()
#define LOG_ERROR(x,y) if (MWLOGGER::Logger::GetLogLevel(x) <= MWLOGGER::ERROR) MWLOGGER::Logger(x, y, MWLOGGER::ERROR, __FILE__, __LINE__, __func__).stream()
#define LOG_FATAL(x,y) if (MWLOGGER::Logger::GetLogLevel(x) <= MWLOGGER::FATAL) MWLOGGER::Logger(x, y, MWLOGGER::FATAL, __FILE__, __LINE__, __func__).stream()

}
#endif  

// �÷�����
/*
using namespace MWLOGGER;

//ҵ��ģ����־ö��,��������ID,������ʶһ����־
enum MODULE_LOGS
{
	MODULE_LOGS_DEFAULT,	//Ĭ��ģ��,����ģ��
	MODULE_LOGS_DOWNLOAD,	//���ֲ�ͬ��ģ��.....�ж��ٸ��������־��ģ��,�Ͷ�����ٸ�,ÿ����һ��,�ͻ�����һ����־
	MODULE_LOG_NUMS,		//��־����
};			
//���岻ͬģ�����־����,������MODULE_LOGS�г�MODULE_LOG_NUMS��һһ��Ӧ!!!
//ÿ������һ����־ʱg_module_logs��MODULE_LOGS��Ҫ��Ӧ�ļӽ���Ӧ��ֵ
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
