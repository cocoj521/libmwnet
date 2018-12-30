#include "FileMgr.h"

#include <unistd.h>
#include "DirUtil.h"

namespace MSGFILES
{

FileMgr::FileMgr()
	: mutexOkFile_(),
	  mutexBakFile_(),
	  mutexCurFile_(),
	  fp_(NULL),
	  maxFileSize_(20),
	  genFreq_(180),
	  yyyymmdd_(0),
	  hour_(0),
	  fileId_(0),
	  curPos_(0),
	  lastSwitch_(0),
	  bApply_(false)
{
	yyyymmdd_ = GenDirId(&hour_);
	fileId_ = GenFileId();
}

FileMgr::~FileMgr()
{
	std::lock_guard<std::mutex> lock(mutexCurFile_);
	CloseCurFile();
}

int	FileMgr::ApplyFileInfo(const std::string& szMainPath, int maxFileSize, int genFreq)
{
	std::lock_guard<std::mutex> lock(mutexCurFile_);
	if (bApply_) return 0;

	maxFileSize < 1 ? maxFileSize = 1 : 1;
	maxFileSize > 1024 ? maxFileSize = 1024 : 1;
	genFreq < 3 ? genFreq = 180 : 1;

	maxFileSize_ = maxFileSize;
	genFreq_ = genFreq;
	path_ = szMainPath;

	yyyymmdd_ = GenDirId(&hour_);
	fileId_   = GenFileId();
	fp_ = GetFilePtr(path_.c_str(), yyyymmdd_, hour_, fileId_);

	// 记录时间
	time(&lastSwitch_);
	if (fp_)
	{
		bApply_ = true;
	}
	return bApply_ ? 0 : -1;
}

void FileMgr::SetParams(int genFreq, int maxFileSize)
{
	maxFileSize < 1 ? maxFileSize = 1 : 1;
	maxFileSize > 1024 ? maxFileSize = 1024 : 1;

	maxFileSize_ = maxFileSize;
	genFreq_ = genFreq;
}

// int FileMgr::WriteToFile(const char* content, int size, int& offset, int yyyymmdd, int hour, int& fileId)
int FileMgr::WriteToFile(const char* content, size_t size)
{
	std::lock_guard<std::mutex> lock(mutexCurFile_);
	if (!bApply_) return -1;

	// 根据需要切换文件
	SwitchFile();

	// 返回偏移量等信息
	int nRet = -1;
	// 防止磁盘空间满了，申请的指针为NULL
	if (fp_)
	{
		// 写内容
		if (size == fwrite(content, sizeof(char), size, fp_))
		{
			if (EOF == fflush(fp_))
			{
				// 磁盘空间不够，强制切换文件
				SwitchFile(true);
			}
			else
			{
				curPos_ += static_cast<int>(size);
				nRet = 0;
			}
		}
		else
		{
			// 写入的长度不等时，说明发生了错误，强制切换文件，但不重写，认为这次内容写失败了
			printf("写文件失败,content: %s, size: %ld", content, size);
			SwitchFile(true);
		}
	}
	else
	{
		printf("文件指针为NULL, content: %s, size: %ld", content, size);
		// 文件指针为NULL，可能没有磁盘了，要强制切换，重新申请，否则磁盘恢复后又得等3分钟才能切换
		SwitchFile(true);
	}

	return nRet;
}

int FileMgr::ReadFromFile(int yyyymmdd, int hour, int fileId, int offset, int size, char* content)
{
	if (!bApply_) return 0;

	int nRet = 0;
	// 如果是当前文件则直接取之
	// 如果是OK文件则打开取之
	// 如果是bak文件则打开取之
	if (!SearchCurrentFile(yyyymmdd, hour, fileId, offset, size, content) &&
	        !SearchOkFile(yyyymmdd, hour, fileId, offset, size, content) &&
	        !SearchBakFile(yyyymmdd, hour, fileId, offset, size, content))
	{
		nRet = -1;
	}
	return nRet;
}

void FileMgr::SwitchToNewFile()
{
	if (bApply_)
	{
		std::lock_guard<std::mutex> lock(mutexCurFile_);
		SwitchFile();
	}
}

void FileMgr::Close()
{
	std::lock_guard<std::mutex> lock(mutexCurFile_);
	if (fp_)
		fclose(fp_);
}

int FileMgr::RenameFile(const char* oldName, const char* newName)
{
	return rename(oldName, newName);
}

FILE* FileMgr::GetFilePtr(const char* fileDir, int yyyymmdd, int hour, int fileId)
{
	char filePath[256] = { 0 };
	// YYYY
	size_t len = snprintf(filePath, sizeof(filePath), "%s/%04d", fileDir, yyyymmdd / 10000);
	// 判断目录是否存在，不存在就创建
	if (common::DirUtil::MakeDir(filePath) == -1)
	{
		printf("创建目录失败: %s", common::DirUtil::GetLastError());
		return NULL;
	}

	// MM
	len += snprintf(filePath + len, sizeof(filePath) - len, "/%02d", (yyyymmdd % 10000) / 100);
	// 判断目录是否存在，不存在就创建
	if (common::DirUtil::MakeDir(filePath) == -1)
	{
		printf("创建目录失败: %s", common::DirUtil::GetLastError());
		return NULL;
	}

	// DD
	len += snprintf(filePath + len, sizeof(filePath) - len, "/%02d", (yyyymmdd % 10000) % 100);
	if (common::DirUtil::MakeDir(filePath) == -1)
	{
		printf("创建目录失败: %s", common::DirUtil::GetLastError());
		return NULL;
	}

	// HH
	len += snprintf(filePath + len, sizeof(filePath) - len, "/%02d", hour);
	if (common::DirUtil::MakeDir(filePath) == -1)
	{
		printf("创建目录失败: %s", common::DirUtil::GetLastError());
		return NULL;
	}

	snprintf(filePath + len, sizeof(filePath) - len, "/%09d.txt", fileId);
	return fopen(filePath, "ab+");
}

void FileMgr::SwitchFile(bool force)
{
	if (!bApply_) return;

	int hour;
	int yyyymmdd = GenDirId(&hour);
	// 判断是否跨天，跨小时，文件大小是否超限，是否超过设定的频次
	if (force || (yyyymmdd != yyyymmdd_) || (yyyymmdd == yyyymmdd_ && hour != hour_) ||
	        curPos_ > maxFileSize_ * 1024 * 1024 || time(NULL) - lastSwitch_ > genFreq_)
	{
		int fileId = GenFileId();
		char szCurName[1024] = { 0 };
		char szOkName[1024] = { 0 };

		snprintf(szCurName, sizeof(szCurName), "%s/%04d/%02d/%02d/%02d/%09d.txt",
		         path_.c_str(), yyyymmdd_ / 10000, (yyyymmdd_ % 10000) / 100,
		         (yyyymmdd_ % 10000) % 100, hour_, fileId_);

		snprintf(szOkName, sizeof(szCurName), "%s/%04d/%02d/%02d/%02d/%09d_ok.txt",
		         path_.c_str(), yyyymmdd_ / 10000, (yyyymmdd_ % 10000) / 100,
		         (yyyymmdd_ % 10000) % 100, hour_, fileId_);

		// 关闭原文件指针
		if (fp_)
		{
			fflush(fp_);
			fclose(fp_);
			fp_ = NULL;
		}
		// 重命名文件
		RenameFile(szCurName, szOkName);

		// 生成一个新文件
		fp_ = GetFilePtr(path_.c_str(), yyyymmdd, hour, fileId);
		// 刷新最后一次切换的时间
		time(&lastSwitch_);

		// 文件信息重置
		yyyymmdd_ = yyyymmdd;
		hour_ = hour;
		fileId_ = fileId;
		curPos_ = 0;
	}
}

void FileMgr::CloseCurFile()
{
	if (!bApply_) return;

	int hour;
	int yyyymmdd = GenDirId(&hour);
	int fileId = GenFileId();

	char szCurName[1024] = { 0 };
	char szOkName[1024] = { 0 };

	snprintf(szCurName, sizeof(szCurName), "%s/%04d/%02d/%02d/%02d/%09d.txt",
	         path_.c_str(), yyyymmdd_ / 10000, (yyyymmdd_ % 10000) / 100,
	         (yyyymmdd_ % 10000) % 100, hour_, fileId_);

	snprintf(szOkName, sizeof(szCurName), "%s/%04d/%02d/%02d/%02d/%09d_ok.txt",
	         path_.c_str(), yyyymmdd_ / 10000, (yyyymmdd_ % 10000) / 100,
	         (yyyymmdd_ % 10000) % 100, hour_, fileId_);

	// 关闭原文件指针
	if (fp_)
	{
		fflush(fp_);
		fclose(fp_);
		fp_ = NULL;
	}
	// 重命名文件
	RenameFile(szCurName, szOkName);

	// 刷新最后一次切换的时间
	time(&lastSwitch_);

	// 文件信息重置
	yyyymmdd_ = yyyymmdd;
	hour_ = hour;
	fileId_ = fileId;
	curPos_ = 0;
}

bool FileMgr::SearchCurrentFile(int yyyymmdd, int hour, int fileId, int offset, int size, char* content)
{
	int bRet = false;
	std::lock_guard<std::mutex> lock(mutexCurFile_);
	if (yyyymmdd == yyyymmdd_ && hour == hour_ && fileId == fileId_)
	{
		// 从当前文件中读取
		// 将文件指针移到指定的偏移量处
		if (0 == fseek(fp_, static_cast<long>(offset), SEEK_SET))
		{
			// 读取指定长度的内容
			if (size == static_cast<int>(fread(content, sizeof(char), size, fp_) && size > 0))
			{
				bRet = true;
				fseek(fp_, 0L, SEEK_END);
			}
		}
	}
	return bRet;
}

bool FileMgr::SearchOkFile(int yyyymmdd, int hour, int fileId, int offset, int size, char* content)
{
	int bRet = false;
	std::lock_guard<std::mutex> lock(mutexOkFile_);
	char szOkFile[1024] = { 0 };
	snprintf(szOkFile, sizeof(szOkFile), "%s/%04d/%02d/%02d/%02d/%09d_ok.txt",
	         path_.c_str(), yyyymmdd / 10000, (yyyymmdd % 10000) / 100,
	         (yyyymmdd % 10000) % 100, hour, fileId);
	FILE* fp = fopen(szOkFile, "rb");
	if (fp)
	{
		if (0 == fseek(fp, static_cast<long>(offset), SEEK_SET))
		{
			// 从历史文件中读取
			if (size == static_cast<int>(fread(content, sizeof(char), size, fp) && size > 0))
			{
				bRet = true;
			}
		}
		fclose(fp);
	}
	return bRet;
}

bool FileMgr::SearchBakFile(int yyyymmdd, int hour, int fileId, int offset, int size, char* content)
{
	int bRet = false;
	std::lock_guard<std::mutex> lock(mutexBakFile_);
	char szOkFile[1024] = { 0 };
	snprintf(szOkFile, sizeof(szOkFile), "%s/%04d/%02d/%02d/%02d/%09d_bak.txt",
	         path_.c_str(), yyyymmdd / 10000, (yyyymmdd % 10000) / 100,
	         (yyyymmdd % 10000) % 100, hour, fileId);
	FILE* fp = fopen(szOkFile, "rb");
	if (fp)
	{
		if (0 == fseek(fp, static_cast<long>(offset), SEEK_SET))
		{
			// 从历史文件中读取
			if (size == static_cast<int>(fread(content, sizeof(char), size, fp) && size > 0))
			{
				bRet = true;
			}
		}
		fclose(fp);
	}
	return bRet;
}

} // end namespace RND
