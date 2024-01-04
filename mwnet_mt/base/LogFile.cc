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
                 int checkEveryN,
				 bool enableZip)
  : basepath_(basepath),
    basename_(basename),
    rollSize_(rollSize),
    flushInterval_(flushInterval),
    checkEveryN_(checkEveryN),
    count_(0),
    mutex_(threadSafe ? new MutexLock : NULL),
	enableZip_(enableZip),
	toZipFileMutex_(enableZip ? new MutexLock : NULL),
    startOfPeriod_(0),
    lastRoll_(0),
    lastFlush_(0),
	nowlogfile_("")
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

//创建多级目录
bool mkdirs(const std::string& filename, mode_t mode)
{
	bool ret = false;

	//循环创建
	size_t pos = 0;
	while ((pos = filename.find('/', pos)) != std::string::npos)
	{
		std::string dir = filename.substr(0, pos);
		if (!dir.empty() && dir != ".")
		{
			//不存在则创建
			if (0 != access(dir.c_str(), R_OK) && 0 != mkdir(dir.c_str(), mode))
			{
				//创建失败
				ret = false;
				break;
			}
		}
		++pos;
	}

	return ret;
}

bool LogFile::rollFile()
{
  time_t now = time(NULL);  

  if (now != lastRoll_)
  {
   	string filename = getNewLogFileName(basepath_, basename_, now);
	time_t start = (now + k8HourToSeconds_) / kRollPerSeconds_ * kRollPerSeconds_;
	
    lastRoll_ = now;
    lastFlush_ = now;
    startOfPeriod_ = start;
    
    string createDir;
    std::size_t pos = filename.find_last_of('/');
    if (pos != string::npos)
    {
	  //printf("%s\n", filename.c_str());
      //createDir = filename.substr(0, pos);
      //mkdir(createDir.c_str(), 0755);
		mkdirs(filename.c_str(), 0755);
    }
    // std::cout << "filename = " << filename << ", createDir = " << createDir << std::endl;
    file_.reset(new FileUtil::AppendFile(filename));

	// 将旧文件丢入压缩队列
	//zip -m /aa/bb/cc/xx.zip /aa/bb/cc/xx.log
	addToZipList(nowlogfile_);

	// 保存新文件全路径
	nowlogfile_ = filename;

    return true;
  }
  return false;
}

void LogFile::addToZipList(const string& logfilename)
{
	if (enableZip_ && toZipFileMutex_ && !logfilename.empty())
	{
		std::size_t pos = logfilename.find_last_of('/');
		if (pos != std::string::npos)
		{
			_file_to_zip toZip;
			toZip.t_ = time(NULL);
			toZip.file_path_ = logfilename.substr(0, pos);
			toZip.file_name_ = logfilename.substr(pos + 1);
			MutexLockGuard lock(*toZipFileMutex_);
			zipFileList_.push_back(toZip);
		}
	}
}

int LogFile::getToZipFile(int minutesAgo, string& filePath, string& fileName)
{
	int ret = -1;
	if (enableZip_ && toZipFileMutex_)
	{
		MutexLockGuard lock(*toZipFileMutex_);
		std::list<_file_to_zip>::iterator it = zipFileList_.begin();
		if (it != zipFileList_.end() && time(NULL) - it->t_ >= minutesAgo*60)
		{
			filePath = it->file_path_;
			fileName = it->file_name_;
			zipFileList_.erase(it);
			ret = 0;
		}
	}
	return ret;
}

string LogFile::getNewLogFileName(const string& basepath, const string& basename, time_t now)
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

