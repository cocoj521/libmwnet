#include <mwnet_mt/base/LogFile.h>

#include <mwnet_mt/base/FileUtil.h>
#include <mwnet_mt/base/ProcessInfo.h>

#include <assert.h>
#include <stdio.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/types.h>

// #include <iostream>

using namespace mwnet_mt;

LogFile::LogFile(const string& basepath,
                 const string& basename,
                 size_t rollSize,
                 bool threadSafe,
                 int flushInterval,
                 int checkEveryN)
  : basepath_(basepath),
    basename_(basename),
    rollSize_(rollSize),
    flushInterval_(flushInterval),
    checkEveryN_(checkEveryN),
    count_(0),
    mutex_(threadSafe ? new MutexLock : NULL),
    startOfPeriod_(0),
    lastRoll_(0),
    lastFlush_(0)
{
  assert(basename.find('/') == string::npos);
  rollFile();
}

LogFile::~LogFile()
{
}

void LogFile::append(const char* logline, size_t len)
{
  if (mutex_)
  {
    MutexLockGuard lock(*mutex_);
    append_unlocked(logline, len);
  }
  else
  {
    append_unlocked(logline, len);
  }
}

void LogFile::flush()
{
  if (mutex_)
  {
    MutexLockGuard lock(*mutex_);
    file_->flush();
  }
  else
  {
    file_->flush();
  }
}

void LogFile::append_unlocked(const char* logline, size_t len)
{
  file_->append(logline, len);

  if (file_->writtenBytes() > rollSize_)
  {
    rollFile();
  }
  else
  {
    ++count_;
    if (count_ >= checkEveryN_)
    {
      count_ = 0;
      time_t now = ::time(NULL);
      time_t thisPeriod_ = (now + k8HourToSeconds_)/ kRollPerSeconds_ * kRollPerSeconds_;
      if (thisPeriod_ != startOfPeriod_)
      {
        rollFile();
      }
      else if (now - lastFlush_ > flushInterval_ || 0 == flushInterval_)
      {
        lastFlush_ = now;
        file_->flush();
      }
    }
  }
}

bool LogFile::rollFile()
{
  time_t now = time(NULL);  

  if (now > lastRoll_)
  {
   	string filename = getLogFileName(basepath_, basename_, now);
	time_t start = (now + k8HourToSeconds_) / kRollPerSeconds_ * kRollPerSeconds_;
	
    lastRoll_ = now;
    lastFlush_ = now;
    startOfPeriod_ = start;
    
    string createDir;
    std::size_t pos = filename.find_last_of('/');
    if (pos != string::npos)
    {
      createDir = filename.substr(0, pos);
      mkdir(createDir.c_str(), 0755);
    }
    // std::cout << "filename = " << filename << ", createDir = " << createDir << std::endl;
    // sleep(5);
    file_.reset(new FileUtil::AppendFile(filename));
    return true;
  }
  return false;
}

string LogFile::getLogFileName(const string& basepath, const string& basename, time_t now)
{
  string filename;
  filename.reserve(basepath.size() + basename.size() + 64);
  filename = basepath;

  char datebuf[16];
  char timebuf[16];
  
  struct tm tm;
  localtime_r(&now, &tm); 
  strftime(datebuf, sizeof datebuf, "%Y%m%d/", &tm);
  strftime(timebuf, sizeof timebuf, "-%H%M%S", &tm);

  filename.append(datebuf).append(basename).append(timebuf).append(".log");
  return filename;
}

