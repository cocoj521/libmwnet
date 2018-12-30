#ifndef MWNET_MT_BASE_LOGGING_H
#define MWNET_MT_BASE_LOGGING_H

#include <mwnet_mt/base/LogStream.h>
#include <mwnet_mt/base/Timestamp.h>

namespace mwnet_mt
{

class TimeZone;

class Logger
{
 public:
  enum LogLevel
  {
    TRACE,
    DEBUG,
    INFO,
    WARN,
    ERROR,
    FATAL,
    NUM_LOG_LEVELS,
  };

  // compile time calculation of basename of source file
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

  Logger(SourceFile file, int line, const char* func);
  Logger(SourceFile file, int line, LogLevel level, const char* func);
  Logger(SourceFile file, int line, bool toAbort, const char* func);
  ~Logger();

  LogStream& stream() { return impl_.stream_; }

  static LogLevel logLevel();
  static void setLogLevel(LogLevel level);

  typedef void (*OutputFunc)(const char* msg, int len);
  typedef void (*FlushFunc)();
  static void setOutput(OutputFunc);
  static void setFlush(FlushFunc);
  static void setTimeZone(const TimeZone& tz);

 private:

class Impl
{
 public:
  typedef Logger::LogLevel LogLevel;
  Impl(LogLevel level, int old_errno, const SourceFile& file, int line, const std::string& func);
  void formatTime();
  void finish();

  Timestamp time_;
  LogStream stream_;
  LogLevel level_;
  int line_;
  SourceFile basename_;
  std::string func_;
};

  Impl impl_;

};

extern Logger::LogLevel g_logLevel;

inline Logger::LogLevel Logger::logLevel()
{
  return g_logLevel;
}

#define LOG_TRACE if (mwnet_mt::Logger::logLevel() <= mwnet_mt::Logger::TRACE) \
  mwnet_mt::Logger(__FILE__, __LINE__, mwnet_mt::Logger::TRACE, __func__).stream()
#define LOG_DEBUG if (mwnet_mt::Logger::logLevel() <= mwnet_mt::Logger::DEBUG) \
  mwnet_mt::Logger(__FILE__, __LINE__, mwnet_mt::Logger::DEBUG, __func__).stream()
#define LOG_INFO if (mwnet_mt::Logger::logLevel() <= mwnet_mt::Logger::INFO) \
  mwnet_mt::Logger(__FILE__, __LINE__, __func__).stream()
#define LOG_WARN mwnet_mt::Logger(__FILE__, __LINE__, mwnet_mt::Logger::WARN, __func__).stream()
#define LOG_ERROR mwnet_mt::Logger(__FILE__, __LINE__, mwnet_mt::Logger::ERROR, __func__).stream()
#define LOG_FATAL mwnet_mt::Logger(__FILE__, __LINE__, mwnet_mt::Logger::FATAL, __func__).stream()
#define LOG_SYSERR mwnet_mt::Logger(__FILE__, __LINE__, false, __func__).stream()
#define LOG_SYSFATAL mwnet_mt::Logger(__FILE__, __LINE__, true, __func__).stream()

const char* strerror_tl(int savedErrno);

// Taken from glog/logging.h
//
// Check that the input is non NULL.  This very useful in constructor
// initializer lists.

#define CHECK_NOTNULL(val) \
  ::mwnet_mt::CheckNotNull(__FILE__, __LINE__, "'" #val "' Must be non NULL", (val))

// A small helper for CHECK_NOTNULL().
template <typename T>
T* CheckNotNull(Logger::SourceFile file, int line, const char *names, T* ptr)
{
  if (ptr == NULL)
  {
   Logger(file, line, Logger::FATAL, __func__).stream() << names;
  }
  return ptr;
}

}

#endif  // MWNET_MT_BASE_LOGGING_H
