#ifndef __MSGFILES_FILEMGR_H__
#define __MSGFILES_FILEMGR_H__

#include <time.h>
#include <sys/time.h>
#include <string>
#include <mutex>


typedef struct tm SystemTime;

namespace MSGFILES
{

class FileMgr
{
public:
	FileMgr();
	~FileMgr();

	int	ApplyFileInfo(const std::string& szMainPath, int maxFileSize, int genFreq);

	void SetParams(int genFreq, int maxFileSize);

	// int WriteToFile(const char* content, int size, int& offset, int yyyymmdd, int hour, int& fileId);
	int WriteToFile(const char* content, size_t size);

	int ReadFromFile(int yyyymmdd, int hour, int fileId, int offset, int size, char* content);

	void SwitchToNewFile();

	void Close();

private:
	inline int RenameFile(const char* oldName, const char* newName);

	inline int GenFileId()
	{
		struct timeval tv;
		gettimeofday(&tv, NULL);
		struct tm tm_time;

		localtime_r(&(tv.tv_sec), &tm_time);

		return static_cast<int>(tm_time.tm_hour * 10000000 +
		                        tm_time.tm_min * 100000 +
		                        tm_time.tm_sec * 1000 +
		                        tv.tv_usec / 1000);
	}

	inline int GenDirId(int* hour)
	{
		time_t seconds = time(NULL);
		struct tm tm_time;
		localtime_r(&seconds, &tm_time);
		*hour = tm_time.tm_hour;
		return (tm_time.tm_year + 1900) * 10000 + (tm_time.tm_mon + 1) * 100 + tm_time.tm_mday;
	}

	FILE* GetFilePtr(const char* fileDir, int yyyymmdd, int hour, int fileId);
	void SwitchFile(bool force = false);

	void CloseCurFile();

	bool SearchCurrentFile(int yyyymmdd, int hour, int fileId, int offset, int size, char* content);
	bool SearchOkFile(int yyyymmdd, int hour, int fileId, int offset, int size, char* content);
	bool SearchBakFile(int yyyymmdd, int hour, int fileId, int offset, int size, char* content);

private:
	std::mutex mutexOkFile_;
	std::mutex mutexBakFile_;
	std::mutex mutexCurFile_;

	std::string path_;
	FILE* fp_;
	int maxFileSize_;
	int genFreq_;
	int yyyymmdd_;
	int hour_;
	int fileId_;
	int curPos_;
	time_t lastSwitch_;
	bool bApply_;
};

} // end namespace MSGFILES

#endif // __MSGFILES_FILEMGR_H__
