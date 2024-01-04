#ifndef MWNET_MT_BASE_LOGFILE_H
#define MWNET_MT_BASE_LOGFILE_H

#include <mwnet_mt/base/Mutex.h>
#include <mwnet_mt/base/Types.h>
#include <memory>
#include <list>

namespace mwnet_mt
{

namespace FileUtil
{
class AppendFile;
}

//需要压缩的文件
typedef struct tFileToZip
{
	string file_path_;//文件路径
	string file_name_;//文件名
	time_t t_;//文件添加时间
}_file_to_zip;

class LogFile : noncopyable
{
 public:
  LogFile(const string& basepath,
          const string& basename,
          size_t rollSize,
          bool threadSafe = true,
          int flushInterval = 3,
          int checkEveryN = 1024,
		  bool enableZip = false);
  ~LogFile();

  void append(const char* logline, size_t len);
  void flush();
  bool rollFile();
  // 返回xx分钟前需要压缩的文件
  int getToZipFile(int minutesAgo, string& filePath, string& fileName);
 private:
  void append_unlocked(const char* logline, size_t len);
  
  void addToZipList(const string& logfilename);

  static string getNewLogFileName(const string& basepath, const string& basename, time_t now);
  
  const string basepath_;
  const string basename_;
  const size_t rollSize_;
  const int flushInterval_;
  const int checkEveryN_;

  int count_;

  std::unique_ptr<MutexLock> mutex_;
  bool enableZip_;
  std::unique_ptr<MutexLock> toZipFileMutex_;
  time_t startOfPeriod_;
  time_t lastRoll_;
  time_t lastFlush_;
  string nowlogfile_;//当前日志文件全路径

  std::unique_ptr<FileUtil::AppendFile> file_;
  std::list<_file_to_zip> zipFileList_;
  const static int kRollPerSeconds_ = 60*60*24;
  const static int k8HourToSeconds_ = 60*60*8;
};

}
#endif  // MWNET_MT_BASE_LOGFILE_H
