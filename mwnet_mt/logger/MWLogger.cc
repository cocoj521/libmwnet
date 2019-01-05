#include <mwnet_mt/base/CurrentThread.h>
#include <mwnet_mt/base/TimeZone.h>
#include <mwnet_mt/base/LogStream.h>
#include <mwnet_mt/base/Timestamp.h>
#include <mwnet_mt/base/StringPiece.h>
#include <mwnet_mt/base/Types.h>
#include <mwnet_mt/base/LogFile.h>
#include <mwnet_mt/base/AsyncLogging.h>
#include <assert.h>
#include <string.h> // memcpy
#include <string>
#include <vector>
#include <errno.h>
#include <stdio.h>
#include <sys/time.h>
#include <sstream>
#include <algorithm>
#include <sys/stat.h>
#include <sys/types.h>
#include <stdarg.h>
#include <time.h>
#include <stdio.h>

#include "MWLogger.h"

#if defined(__clang__)
#pragma clang diagnostic ignored "-Wtautological-compare"
#else
#pragma GCC diagnostic ignored "-Wtype-limits" 
#endif

using namespace MWLOGGER;
using namespace MWLOGSTREAM;

namespace MWLOGGER
{
namespace TMPUTIL
{
const char digits[] = "9876543210123456789";
const char* zero = digits + 9;
static_assert(sizeof(digits) == 20, "wrong number of digits");

const char digitsHex[] = "0123456789ABCDEF";
static_assert(sizeof digitsHex == 17, "wrong number of digitsHex");

// Efficient Integer to String Conversions, by Matthew Wilson.
template<typename T>
size_t convert(char buf[], T value)
{
	T i = value;
	char* p = buf;

	do
	{
		int lsd = static_cast<int>(i % 10);
		i /= 10;
		*p++ = zero[lsd];
	} while (i != 0);

	if (value < 0)
	{
		*p++ = '-';
	}
	*p = '\0';
	std::reverse(buf, p);

	return p - buf;
}

size_t convertHex(char buf[], uintptr_t value)
{
	uintptr_t i = value;
	char* p = buf;

	do
	{
		int lsd = static_cast<int>(i % 16);
		i /= 16;
		*p++ = digitsHex[lsd];
	} while (i != 0);

	*p = '\0';
	std::reverse(buf, p);

	return p - buf;
}
}

namespace MWLOGSTREAM
{
const int kMaxNumericSize = 32;
const int kInitStreamBuf  = 32*1024;

void staticCheck()
{
  static_assert(kMaxNumericSize - 10 > std::numeric_limits<double>::digits10,
                "kMaxNumericSize is large enough");
  static_assert(kMaxNumericSize - 10 > std::numeric_limits<long double>::digits10,
                "kMaxNumericSize is large enough");
  static_assert(kMaxNumericSize - 10 > std::numeric_limits<long>::digits10,
                "kMaxNumericSize is large enough");
  static_assert(kMaxNumericSize - 10 > std::numeric_limits<long long>::digits10,
                "kMaxNumericSize is large enough");
}

LogStream::LogStream()
{
	m_LogStrPtr.reset(new std::string());
	m_LogStrPtr->reserve(kInitStreamBuf);
}

LogStream& LogStream::Format(const char * _Format, ...)
{
	if (m_LogStrPtr)
	{
		va_list arg_list;

		// SUSv2 version doesn't work for buf NULL/size 0, so try printing
		// into a small buffer that avoids the double-rendering and alloca path too...
		char short_buf[512] = { 0 };

		va_start(arg_list, _Format);
		const size_t needed = vsnprintf(short_buf, sizeof short_buf,
			_Format, arg_list);
		va_end(arg_list);

		if (needed < sizeof short_buf)
		{
			m_LogStrPtr->append(short_buf, needed);
		}
		else
		{
			// need more space...
			char* pStr = static_cast<char*>(alloca(needed+1));

			va_start(arg_list, _Format);
			vsnprintf(pStr, needed, _Format, arg_list);
			va_end(arg_list);

			m_LogStrPtr->append(pStr, needed);
		}	
	}
	return *this;
}

template<typename T>
void LogStream::formatInteger(T v)
{
	char buf[kMaxNumericSize] = {0};
	size_t len = TMPUTIL::convert(buf, v);
	if (m_LogStrPtr) m_LogStrPtr->append(buf, len);
}

void LogStream::append(const char* data, size_t len) 
{ 
	if (m_LogStrPtr) m_LogStrPtr->append(data, len);
}

const LogStringPtr& LogStream::getbuffer() const 
{ 
	return m_LogStrPtr; 
}

void LogStream::resetbuffer() 
{ 
	if (m_LogStrPtr) m_LogStrPtr->resize(0); 
}

LogStream& LogStream::operator<<(bool v)
{
	if (m_LogStrPtr) m_LogStrPtr->append(v ? "1" : "0", 1);
	return *this;
}

LogStream& LogStream::operator<<(float v)
{
	*this << static_cast<double>(v);
	return *this;
}

LogStream& LogStream::operator<<(char v)
{
	if (m_LogStrPtr) m_LogStrPtr->append(&v, 1);
	return *this;
}

LogStream& LogStream::operator<<(const char* str)
{
	if (str)
	{
		if (m_LogStrPtr) m_LogStrPtr->append(str, strlen(str));
	}
	else
	{
		if (m_LogStrPtr) if (m_LogStrPtr) m_LogStrPtr->append("(null)", 6);
	}
	return *this;
}

LogStream& LogStream::operator<<(const unsigned char* str)
{
	return operator<<(reinterpret_cast<const char*>(str));
}

LogStream& LogStream::operator<<(const std::string& v)
{
	if (m_LogStrPtr) m_LogStrPtr->append(v.c_str(), v.size());
	return *this;
}

LogStream& LogStream::operator<<(short v)
{
	*this << static_cast<int>(v);
	return *this;
}

LogStream& LogStream::operator<<(unsigned short v)
{
	*this << static_cast<unsigned int>(v);
	return *this;
}

LogStream& LogStream::operator<<(int v)
{
	formatInteger(v);
	return *this;
}

LogStream& LogStream::operator<<(unsigned int v)
{
	formatInteger(v);
	return *this;
}

LogStream& LogStream::operator<<(long v)
{
	formatInteger(v);
	return *this;
}

LogStream& LogStream::operator<<(unsigned long v)
{
	formatInteger(v);
	return *this;
}

LogStream& LogStream::operator<<(long long v)
{
	formatInteger(v);
	return *this;
}

LogStream& LogStream::operator<<(unsigned long long v)
{
	formatInteger(v);
	return *this;
}

LogStream& LogStream::operator<<(const void* p)
{
	uintptr_t v = reinterpret_cast<uintptr_t>(p);
	char buf[kMaxNumericSize] = {0};
	buf[0] = '0';
	buf[1] = 'x';
	size_t len = TMPUTIL::convertHex(buf+2, v);
	if (m_LogStrPtr) m_LogStrPtr->append(buf, len+2);
	
	return *this;
}

// FIXME: replace this with Grisu3 by Florian Loitsch.
LogStream& LogStream::operator<<(double v)
{
	char buf[kMaxNumericSize] = {0};
	int len = snprintf(buf, kMaxNumericSize, "%.12g", v);
	if (m_LogStrPtr) m_LogStrPtr->append(buf, len);

	return *this;
}
}

//////////////////////////////////////////////////////////////////////////////////////

class SourceFile
{
public:
	template<int N>
	inline SourceFile(const char (&arr)[N])
		: data_(arr),
		  size_(N-1)
	{
		const char* slash = strrchr(data_, '/'); // builtin function
		if (slash)
		{
			data_ = slash + 1;
			size_ -= static_cast<int>(data_ - arr);
		}
	}

	explicit SourceFile(const char* filename)
		: data_(filename)
	{
		const char* slash = strrchr(filename, '/');
		if (slash)
		{
			data_ = slash + 1;
		}
		size_ = static_cast<int>(strlen(data_));
	}

	const char* data_;
	int size_;
};


// helper class for known string length at compile time
class T
{
public:
	T(const char* str, unsigned len)
		: str_(str),
	 	  len_(len)
	{
		assert(strlen(str) == len_);
	}

	const char* str_;
	const unsigned len_;
};

inline MWLOGSTREAM::LogStream& operator<<(MWLOGSTREAM::LogStream& s, T v)
{
	s.append(v.str_, v.len_);
	return s;
}

inline MWLOGSTREAM::LogStream& operator<<(MWLOGSTREAM::LogStream& s, const SourceFile& v)
{
	s.append(v.data_, v.size_);
	return s;
}

//////////////////////////////////////////////////////////////////////////////
const char* LogLevelName[NUM_LOG_LEVELS] =
{
  "[TRACE] ",
  "[DEBUG] ",
  "[INFO ] ",
  "[WARN ] ",
  "[ERROR] ",
  "[FATAL] ",
};

const char* strerror_tl(int savedErrno)
{
	char t_errnobuf[512];
	return strerror_r(savedErrno, t_errnobuf, sizeof t_errnobuf);
}

class Impl
{
public:
	Impl(MWLOGGER::LOG_LEVEL level, int old_errno, const SourceFile& file, int line, const std::string& func);
public:
	void formatTime();
	void finish();
public:
	MWLOGSTREAM::LogStream stream_;
	MWLOGGER::LOG_LEVEL level_;
private:
	mwnet_mt::Timestamp time_;
	int line_;
	std::string func_;
	SourceFile basename_;
};

Impl::Impl(MWLOGGER::LOG_LEVEL level, int savedErrno, const SourceFile& file, int line, const std::string& func)
  : stream_(),
  	level_(level),
  	time_(mwnet_mt::Timestamp::now()),
    line_(line),
    func_(func),
    basename_(file)
{
	formatTime();
	stream_ << T(LogLevelName[level], 8);
	mwnet_mt::CurrentThread::tid();
	stream_ << '[' <<T(mwnet_mt::CurrentThread::tidString(), mwnet_mt::CurrentThread::tidStringLength())<<"] ";
	stream_ << '[' << basename_ << "::" << func << ',' << line_ << "]" ;
	if (savedErrno != 0)
	{
		stream_ << ' ' << strerror_tl(savedErrno) << " (errno:" << savedErrno << ')';
	}
	stream_ << ' ';
}

void Impl::formatTime()
{
	char t_time[64];
    struct timeval tv;
    struct tm      t;
    gettimeofday(&tv, NULL);
    localtime_r(&tv.tv_sec, &t);
	sprintf(t_time, 
		"%04d-%02d-%02d %02d:%02d:%02d.%03d ", 
		t.tm_year + 1900, 
		t.tm_mon + 1, 
		t.tm_mday, 
		t.tm_hour, 
		t.tm_min, 
		t.tm_sec, 
		static_cast<int>(tv.tv_usec/1000));
	
	stream_ << t_time;
}

void Impl::finish()
{
  stream_ << '\n';
}
//////////////////////////////////////////////////////////////////////////////////////////////

typedef void (*FUNC_OUTPUT)(int x, OUTPUT_TYPE optype, const LogStringPtr& pLogStrPtr);
typedef void (*FUNC_FLUSH)(int x, OUTPUT_TYPE optype);

MWLOGGER::LOG_LEVEL initLogLevel()
{
  if (::getenv("MWLOGGER_LOG_TRACE"))
    return MWLOGGER::TRACE;
  else if (::getenv("MWLOGGER_LOG_DEBUG"))
    return MWLOGGER::DEBUG;
  else
    return MWLOGGER::INFO;
}

int g_flush_freq = 3;
int g_flush_every = 1000;
int g_roll_size = 100;
int g_zip_day = 3;
int g_save_day = 7;
bool g_bAsyncLog = false;

std::vector<std::string> g_vLogFiles;

struct LOGOBJ_INFO
{
	std::string name_;
	MWLOGGER::LOG_LEVEL level_;
	std::shared_ptr<mwnet_mt::LogFile> logfileptr_;
};

struct ASYNC_LOGOBJ_INFO
{
	std::string name_;
	MWLOGGER::LOG_LEVEL level_;
	std::shared_ptr<mwnet_mt::AsyncLogging> asynLogfileptr_;
};


typedef std::vector<LOGOBJ_INFO>  LOGOBJ_ARRAY;
LOGOBJ_ARRAY g_arrLogs;

typedef std::vector<ASYNC_LOGOBJ_INFO>  ASYNC_LOGOBJ_ARRAY;
ASYNC_LOGOBJ_ARRAY g_arrASyncLogs;


class DealLogFile
{
public:
	DealLogFile()
	{
		m_nZipDay = 3;
		m_nSaveDay = 7;
		m_strBasePath = ".";
		m_strBakPath = ".";

		m_bExit = false;
		m_IsCross = true;
		m_ptrDealFileJob = nullptr;
	};

	void SetParams(int nZipDay, int nSaveDay, const std::string& strBakPath, const std::string& strBasePath, std::vector<std::string>& vFileNames)
	{
		m_nZipDay = nZipDay;
		m_nSaveDay = nSaveDay;
		m_strBasePath = strBasePath;
		m_strBakPath = strBakPath;
		m_vFileNames.swap(vFileNames);

		m_bExit = false;
		m_IsCross = true;
		m_ptrDealFileJob = nullptr;
	}

	bool StartJob()
	{
		m_ptrDealFileJob.reset(new std::thread(&DealLogFile::ThreadForDealFileJob, this));

		if (m_ptrDealFileJob)
		{
			return true;
		}

		return false;
	}

	bool Exit()
	{
		if (m_ptrDealFileJob)
		{
			m_bExit = true;
			m_ptrDealFileJob->join();
		}

		return true;
	}

private:
	//处理日志
	static void ThreadForDealFileJob(void *p)
	{
		DealLogFile *pThis = static_cast<DealLogFile *>(p);

		while (!pThis->m_bExit)
		{
			pThis->DoJob();

			// 休眠5秒
			sleep(5);

		}
	}

	bool DoJob()
	{
		if (IsCrossDay())
		{
			for (auto it : m_vFileNames)
			{
				std::string strZipFile = GetZipFileName(m_strBakPath + it + "/", it);

				//创建对应目录
				std::string createDir;
				std::size_t pos = strZipFile.find_last_of('/');
				if (pos != std::string::npos)
				{
					createDir = strZipFile.substr(0, pos);
					mkdir(createDir.c_str(), 0755);
				}

				std::string str = "find " + m_strBasePath + it + " -mtime +" + std::to_string(m_nZipDay) + " -name \"*.log\" -exec zip -r " + strZipFile + " {} \\; "  //压缩目录中前N天的日志到指定目录
					"&& find " + m_strBasePath + it + " -mtime +" + std::to_string(m_nZipDay) + " -name \"*.log\" -exec rm {} -rf \\; "								   //删除目录中前N天的日志文件
					"&& find " + m_strBasePath + it + " -name \"*\" -type d -empty | xargs  rm -rf \\; "															   //删除目录中空文件夹						
					"&& find " + m_strBakPath + it + " -mtime +" + std::to_string(m_nSaveDay) + " -name \"*.zip\" -exec rm {} -rf \\; "								   //删除备份目录中前N的压缩文件
					"&& find " + m_strBakPath + it + " -name \"*\" -type d -empty | xargs  rm -rf";																	   //删除备份目录中的空文件夹

				//执行任务
				system(str.c_str());
			}
		}

		return true;
	}

	// 是否跨天
	static bool IsCrossDay()
	{
		static int nDay = 0;

		bool bRet = false;
		struct timeval tv;
		struct tm tm_time;

		gettimeofday(&tv, NULL);
		localtime_r(&tv.tv_sec, &tm_time);

		//每天的凌晨执行任务
		if (nDay != tm_time.tm_mday && 0 == tm_time.tm_hour)
		{
			nDay = tm_time.tm_mday;
			bRet = true;
		}

		return bRet;
	}

	static std::string GetZipFileName(const std::string& basepath, const std::string& basename)
	{
		time_t now = time(NULL);

		std::string filename;
		filename.reserve(basepath.size() + basename.size() + 64);
		filename = basepath;

		char datebuf[16];
		char timebuf[16];

		struct tm tm;
		localtime_r(&now, &tm);
		strftime(datebuf, sizeof datebuf, "%Y%m%d/", &tm);
		strftime(timebuf, sizeof timebuf, "-%H%M%S", &tm);

		filename.append(datebuf).append(basename).append(timebuf).append(".zip");
		return filename;
	}

private:
	bool m_bExit;								//退出标志

	int m_nZipDay;								//压缩多少前的日志
	int m_nSaveDay;								//删除多少前的日志
	bool m_IsCross;								//跨天标志
	std::string m_strBasePath;					//要处理文件的路径
	std::string m_strBakPath;					//要备份的路径
	std::vector<std::string> m_vFileNames;		//要处理的日志路径
	std::shared_ptr<std::thread> m_ptrDealFileJob;
};

DealLogFile g_dealLogFile;

void defaultOutput(int x, OUTPUT_TYPE optype, const LogStringPtr& pLogStrPtr)
{
	if (pLogStrPtr)
	{
		switch (optype)
		{
		case LOCAL_FILE:
			{
				if (g_bAsyncLog)
				{
					g_arrASyncLogs[x].asynLogfileptr_->append(pLogStrPtr->c_str(), static_cast<int>(pLogStrPtr->size()));
				}
				else
				{
					g_arrLogs[x].logfileptr_->append(pLogStrPtr->c_str(), pLogStrPtr->size());
				}			
			}
			break;
		case PRINT_STDOUT:
			{
				size_t n = fwrite(pLogStrPtr->c_str(), 1, pLogStrPtr->size(), stdout);
				//FIXME check n
				(void)n;
			}
			break;

		case OUT_ALL:
			{
				size_t n = fwrite(pLogStrPtr->c_str(), 1, pLogStrPtr->size(), stdout);
				//FIXME check n
				(void)n;

				if (g_bAsyncLog)
				{
					g_arrASyncLogs[x].asynLogfileptr_->append(pLogStrPtr->c_str(), static_cast<int>(pLogStrPtr->size()));
				}
				else
				{
					g_arrLogs[x].logfileptr_->append(pLogStrPtr->c_str(), pLogStrPtr->size());
				}	
			}
			break;
		}
	}
}

void defaultFlush(int x, OUTPUT_TYPE optype)
{
	switch (optype)
	{
	case LOCAL_FILE:
		{
			//异步不支持
			if (g_bAsyncLog) return;
			g_arrLogs[x].logfileptr_->flush();
		}
		break;
	case PRINT_STDOUT:
		{
			fflush(stdout);
		}
		break;

	case OUT_ALL:
		{
			//异步不支持
			if (g_bAsyncLog) return;
			g_arrLogs[x].logfileptr_->flush();
			fflush(stdout);
		}
	break;
	}
}

FUNC_OUTPUT g_FuncOutput = defaultOutput;
FUNC_FLUSH	g_FuncFlush = defaultFlush;

typedef std::shared_ptr<Impl> ImplPtr;
LogStream& Logger::stream() 
{ 
	return (boost::any_cast<ImplPtr>(m_any))->stream_;
}

MWLOGGER::LOG_LEVEL Logger::GetLogLevel(int x)
{
	if (g_bAsyncLog)
	{
		return g_arrASyncLogs[x].level_;
	}
	else
	{
		return g_arrLogs[x].level_;
	}
	
}

void Logger::SetLogLevel(MWLOGGER::LOG_LEVEL level)
{
	LOGOBJ_ARRAY::iterator it = g_arrLogs.begin();
	for (; it != g_arrLogs.end(); ++it)
	{
		it->level_ = level;
	}

	ASYNC_LOGOBJ_ARRAY::iterator ita = g_arrASyncLogs.begin();

	for (; ita != g_arrASyncLogs.end(); ++ita)
	{
		ita->level_ = level;
	}
}

void Logger::SetLogLevel(int x, MWLOGGER::LOG_LEVEL level)
{
	if (g_bAsyncLog)
	{
		g_arrASyncLogs[x].level_ = level;
	}
	else
	{
		g_arrLogs[x].level_ = level;
	}

}

void Logger::SetLogParams(int flush_freq, int flush_every, int roll_size, int zip_day, int save_day)
{
	g_flush_freq = flush_freq;
	g_flush_every = flush_every;
	g_roll_size = roll_size;
	g_zip_day = zip_day;
	g_save_day = save_day;
}

bool Logger::CreateLogObjs(const std::string& basepath, const std::string& basedir, int obj_cnt, const char* logs[], bool asynclog)
{
	if (asynclog)
	{
		g_bAsyncLog = asynclog;

		ASYNC_LOGOBJ_INFO asyncLogObj;
		for (int i = 0; i < obj_cnt; ++i)
		{
			asyncLogObj.level_ = MWLOGGER::INFO;
			asyncLogObj.name_ = logs[i];
			g_vLogFiles.push_back(asyncLogObj.name_);

			//创建目录.....
			mkdir((basepath + basedir).c_str(), 0755);
			mkdir((basepath + basedir + asyncLogObj.name_).c_str(), 0755);

			asyncLogObj.asynLogfileptr_.reset(new mwnet_mt::AsyncLogging((basepath + basedir + asyncLogObj.name_ + "/"), asyncLogObj.name_, 1024 * 1024 * g_roll_size, g_flush_freq));

			asyncLogObj.asynLogfileptr_->start();
			g_arrASyncLogs.push_back(asyncLogObj);
		}
	}
	else
	{
		LOGOBJ_INFO logobj;
		for (int i = 0; i < obj_cnt; ++i)
		{
			logobj.level_ = MWLOGGER::INFO;
			logobj.name_ = logs[i];
			g_vLogFiles.push_back(logobj.name_);

			//创建目录.....
			mkdir((basepath + basedir).c_str(), 0755);
			mkdir((basepath + basedir + logobj.name_).c_str(), 0755);

			logobj.logfileptr_.reset(new mwnet_mt::LogFile((basepath + basedir + logobj.name_ + "/"), logobj.name_, 1024 * 1024 * g_roll_size, true, g_flush_freq, g_flush_every));

			g_arrLogs.push_back(logobj);
		}
	}

	std::string strBakPath = basepath + basedir + "BakLog/";
	std::string strBasePath = basepath + basedir;
	mkdir(strBakPath.c_str(), 0755);

	for (auto it : g_vLogFiles)
	{
		mkdir((strBakPath + it).c_str(), 0755);
	}

// 	g_dealLogFile.SetParams(g_zip_day, g_save_day, strBakPath, strBasePath, g_vLogFiles);
// 	g_dealLogFile.StartJob();

	return true;
}
void Logger::Exit()
{
	LOGOBJ_ARRAY::iterator it = g_arrLogs.begin();
	for (; it != g_arrLogs.end(); ++it)
	{
		it->logfileptr_->flush();
	}
	g_dealLogFile.Exit();
}


Logger::Logger(int x, OUTPUT_TYPE optype, LOG_LEVEL level, const char* file, int line, const char* func)
	: m_any(ImplPtr(new Impl(level, 0, SourceFile(file), line, func))),
	  m_optype(optype),
	  m_x(x)
{

}

Logger::~Logger()
{
	ImplPtr pImpl(boost::any_cast<ImplPtr>(m_any));
	
	pImpl->finish();
	
	g_FuncOutput(m_x, m_optype, pImpl->stream_.getbuffer());
	
	if (pImpl->level_ == FATAL)
	{
		g_FuncFlush(m_x, m_optype);
		abort(); 
	}
}

}
