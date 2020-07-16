#include <mwnet_mt/base/Logging.h>

#include <mwnet_mt/base/CurrentThread.h>
#include <mwnet_mt/base/Timestamp.h>
#include <mwnet_mt/base/TimeZone.h>

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <sstream>

namespace mwnet_mt
{

__thread char t_errnobuf[512];
__thread char t_time[64];
__thread time_t t_lastSecond;

const char* strerror_tl(int savedErrno)
{
  return strerror_r(savedErrno, t_errnobuf, sizeof t_errnobuf);
}

Logger::LogLevel initLogLevel()
{
  if (::getenv("MWNET_MT_LOG_TRACE"))
    return Logger::TRACE;
  else if (::getenv("MWNET_MT_LOG_DEBUG"))
    return Logger::DEBUG;
  else
    return Logger::INFO;
}

Logger::LogLevel g_logLevel = initLogLevel();

const char* LogLevelName[Logger::NUM_LOG_LEVELS] =
{
	"[TRACE]",
	"[DEBUG]",
	"[INFO ]",
	"[WARN ]",
	"[ERROR]",
	"[FATAL]",
};

// helper class for known string length at compile time
class T
{
 public:
  T(const char* str, unsigned len)
    :str_(str),
     len_(len)
  {
    assert(strlen(str) == len_);
  }

  const char* str_;
  const unsigned len_;
};

inline LogStream& operator<<(LogStream& s, T v)
{
  s.append(v.str_, v.len_);
  return s;
}

inline LogStream& operator<<(LogStream& s, const Logger::SourceFile& v)
{
  s.append(v.data_, v.size_);
  return s;
}

void defaultOutput(const char* msg, size_t len)
{
  size_t n = fwrite(msg, 1, len, stdout);
  //FIXME check n
  (void)n;
}

void defaultFlush()
{
  fflush(stdout);
}

Logger::LogOutputCallback g_output = defaultOutput;
Logger::LogFlushCallback g_flush = defaultFlush;
//Logger::OutputFunc g_output = defaultOutput;
//Logger::FlushFunc g_flush = defaultFlush;
TimeZone g_logTimeZone;

}

std::string format_file_line(int line)
{
	char buf[10 + 1] = { 0 };
	snprintf(buf, 10, "%4d", line);
	return buf;
}

using namespace mwnet_mt;

Logger::Impl::Impl(LogLevel level, int savedErrno, const SourceFile& file, int line, const std::string& func)
  : time_(Timestamp::now()),
    stream_(),
    level_(level),
    line_(line),
    basename_(file),
    func_(func)
{
  formatTime();
  stream_ << LogLevelName[level];
  CurrentThread::tid();
  stream_ << '[' << T(CurrentThread::tidString(), CurrentThread::tidStringLength())<<"]";
  stream_ << '[' << basename_ << "::" << func << "," << format_file_line(line_) << "]";
  if (savedErrno != 0)
  {
    stream_ << ' ' << strerror_tl(savedErrno) << " (errno=" << savedErrno << ')';
  }

  stream_ << " - ";
}

void Logger::Impl::formatTime()
{
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
/*
  int64_t microSecondsSinceEpoch = time_.microSecondsSinceEpoch();
  time_t seconds = static_cast<time_t>(microSecondsSinceEpoch / Timestamp::kMicroSecondsPerSecond);
  int microseconds = static_cast<int>(microSecondsSinceEpoch % Timestamp::kMicroSecondsPerSecond);
  if (seconds != t_lastSecond)
  {
    t_lastSecond = seconds;
    struct tm tm_time;
    if (g_logTimeZone.valid())
    {
      tm_time = g_logTimeZone.toLocalTime(seconds);
    }
    else
    {
      ::gmtime_r(&seconds, &tm_time); // FIXME TimeZone::fromUtcTime
    }

    int len = snprintf(t_time, sizeof(t_time), "%4d-%02d-%02d %02d:%02d:%02d",
        tm_time.tm_year + 1900, tm_time.tm_mon + 1, tm_time.tm_mday,
        tm_time.tm_hour, tm_time.tm_min, tm_time.tm_sec);
    assert(len == 19); (void)len;
  }

  if (g_logTimeZone.valid())
  {
    Fmt us(".%06d ", microseconds);
    assert(us.length() == 8);
    stream_ << T(t_time, 19) << T(us.data(), 8);
  }
  else
  {
    Fmt us(".%06dZ ", microseconds);
    assert(us.length() == 9);
    stream_ << T(t_time, 19) << T(us.data(), 9);
  }
*/
}

void Logger::Impl::finish()
{
  //stream_ << " - " << basename_ << ':' << line_ << '\n';
  stream_ << '\n';
}

Logger::Logger(SourceFile file, int line, const char* func)
  : impl_(INFO, 0, file, line, func)
{
}

Logger::Logger(SourceFile file, int line, LogLevel level, const char* func)
  : impl_(level, 0, file, line, func)
{
}

Logger::Logger(SourceFile file, int line, bool toAbort, const char* func)
  : impl_(toAbort?FATAL:ERROR, errno, file, line, func)
{
}

Logger::~Logger()
{
  impl_.finish();
  const std::shared_ptr<std::string>& buf = stream().buffer();
  g_output(buf->data(), buf->size());
  if (impl_.level_ == FATAL)
  {
    g_flush();
    abort();
  }
}

void Logger::setLogLevel(Logger::LogLevel level)
{
  g_logLevel = level;
}

void Logger::setOutput(const LogOutputCallback& cb)
{
  g_output = cb;
}

void Logger::setFlush(const LogFlushCallback& cb)
{
  g_flush = cb;
}

void Logger::setTimeZone(const TimeZone& tz)
{
  g_logTimeZone = tz;
}
