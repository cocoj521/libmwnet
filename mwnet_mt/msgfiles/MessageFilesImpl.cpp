#include "MessageFilesImpl.h"

#include <time.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include "DirUtil.h"

namespace MSGFILES
{

const char* kExeNameOk     = "_ok.txt";
const char* kExeNameBak    = "_bak.txt";
const char* kExeNameBakZip = "_bak.zip";

MessageFilesImpl::MessageFilesImpl()
	: bDldFileInit_(false),
	  szMainPath_(""),
	  gateNo_(0),
	  szPtCode_(""),
	  ptCode_(0)
{
	szMainPath_ = common::DirUtil::GetExePath();
	ScanfAndRenameTmpFile();
}

bool MessageFilesImpl::InitParams(const std::string& szPtCode, int ptid, int gateNo)
{
	if (szPtCode.empty() || szPtCode == "")
	{
		return false;
	}

	szPtCode_ = szPtCode;
	ptCode_ = ptid;
	gateNo_ = gateNo;
	return true;
}

void MessageFilesImpl::SetParams(int seconds, int maxFileSize)
{
	std::lock_guard<std::mutex> lock(mutex_);
	for (auto it = details_.begin(); it != details_.end(); ++it)
	{
		if (it->second)
		{
			it->second->SetParams(seconds, maxFileSize);
		}
	}
}

int MessageFilesImpl::InitMsgDetailsFile(const std::string& szPtCode,
        int ptCode,
        int gateNo,
        const std::string& fileType,
        int maxFileSize,
        int genFreq)
{
	std::string pathTmp = szMainPath_;
	// pathTmp.append("/");
	pathTmp.append(MSGFILES_FOLDER_NAME);
	pathTmp.append("/");

	if (szPtCode.empty() || szPtCode == "")
	{
		if (szPtCode_.empty() || szPtCode_ == "")
		{
			return -1;
		}
		pathTmp.append(szPtCode_);
	}
	else
	{
		pathTmp.append(szPtCode);
	}

	pathTmp.append("/");
	pathTmp.append(fileType);
	pathTmp.append("/");
	pathTmp.append(std::to_string(gateNo));

	// 判断目录是否存在，不存在则创建目录
	if (common::DirUtil::MakeDirP(pathTmp) == -1)
	{
		printf("创建目录失败：%s", common::DirUtil::GetLastError());
		return -1;
	}

	if (details_.find(fileType) != details_.end())
	{
		// 已经存在返回成功
		return 0;
	}
	else
	{
		FileMgr* file = new FileMgr;
		if (!file)
		{
			return -1;
		}
		int nRet = file->ApplyFileInfo(pathTmp, maxFileSize, genFreq);
		if (0 == nRet)
		{
			std::lock_guard<std::mutex> lock(mutex_);
			details_.insert(std::make_pair(fileType, file));
		}
		return nRet;
	}
}

bool MessageFilesImpl::bExistFile(const std::string& fileType)
{
	if (fileType.empty() || fileType == "")
	{
		return false;
	}

	{
		std::lock_guard<std::mutex> lock(mutex_);
		if (details_.find(fileType) == details_.end())
		{
			return false;
		}
	}

	return true;
}

int MessageFilesImpl::WriteFile(const std::string& fileType, const std::string& data)
{
	// FIXME: 实现写文件操作
	std::lock_guard<std::mutex> lock(mutex_);
	auto it = details_.find(fileType);
	if (it == details_.end())
	{
		// it = details_.find(szPtCode_);
		return -1;
	}

	if (it != details_.end())
	{
		if (it->second)
		{
			it->second->WriteToFile(data.c_str(), data.length());
		}
	}
	return 0;
}

void MessageFilesImpl::SwitchMessageFiles()
{
	std::lock_guard<std::mutex> lock(mutex_);
	for (auto it = details_.begin(); it != details_.end(); ++it)
	{
		if (it->second)
		{
			it->second->SwitchToNewFile();
		}
	}
}

int MessageFilesImpl::GetLongTimeNoUploadFilesCnt(int min)
{
	int count = 0;
	// 扫描当天的
	count += EnumNoUploadFiles(0, min);
	// 扫描昨天的
	count += EnumNoUploadFiles(1, min);
	return count;
}

void MessageFilesImpl::ScanfAndRenameTmpFile()
{
	std::vector<std::string> tmpFiles;
	std::string pathTmp = szMainPath_ + MSGFILES_FOLDER_NAME;

	EnumTmpFile(pathTmp.c_str(), tmpFiles);
	size_t exNameLen = strlen(FILE_EXNAME);

	for (auto it = tmpFiles.begin(); it != tmpFiles.end(); ++it)
	{
		char buffer[1024] = { 0 };
		memcpy(buffer, it->c_str(), it->size() - exNameLen);
		strcat(buffer, kExeNameOk);
		::rename(it->c_str(), buffer);
	}
}

void MessageFilesImpl::EnumTmpFile(const char* szFolderName, std::vector<std::string>& vFileName)
{
	struct dirent* ent = NULL;
	DIR* pDir = opendir(szFolderName);
	if (pDir)
	{
		while (NULL != (ent = readdir(pDir)))
		{
			// 4表示文件夹, 8表示文件, 过滤掉目录: [.]和[..]
			if (('.' == ent->d_name[0] && 1 == strlen(ent->d_name)) ||
			        ('.' == ent->d_name[0] && '.' == ent->d_name[1] && 2 == strlen(ent->d_name)))
			{
				continue;
			}

			std::string str = szFolderName;
			str.append("/");
			str.append(ent->d_name);

			struct stat st;
			if (0 == stat(str.c_str(), &st))
			{
				if ((st.st_mode & S_IFMT) == S_IFDIR)
				{
					// 文件夹，递归调用扫描文件
					EnumTmpFile(str.c_str(), vFileName);
				}
				else
				{
					std::string::size_type pos = 0;
					// 过滤掉文件名后缀少于4字节的文件
					if ((pos = str.find_last_of('.', str.length())) < 4)
					{
						continue;
					}

					// 过滤掉是以: "_ok.txt" "_bak.txt" "_bak.zip" 的文件
					if (0 != str.compare(pos - 3, 7, kExeNameOk) &&
					        0 != str.compare(pos - 4, 8, kExeNameBak) &&
					        0 != str.compare(pos - 4, 8, kExeNameBakZip))
					{
						vFileName.push_back(str);
					}
				}
			}
		}
		closedir(pDir);
	}
}

void MessageFilesImpl::EnumNoUploadFiles(const char* szFolderName, int nMin, int& nCount)
{
	struct dirent* ent = NULL;
	DIR* pDir = opendir(szFolderName);
	if (pDir)
	{
		while (NULL != (ent = readdir(pDir)))
		{
			// 4表示文件夹, 8表示文件, 过滤掉目录: [.]和[..]
			if (('.' == ent->d_name[0] && 1 == strlen(ent->d_name)) ||
			        ('.' == ent->d_name[0] && '.' == ent->d_name[1] && 2 == strlen(ent->d_name)))
			{
				continue;
			}

			std::string str = szFolderName;
			str.append("/");
			str.append(ent->d_name);

			struct stat st;
			if (0 == stat(str.c_str(), &st))
			{
				if ((st.st_mode & S_IFMT) == S_IFDIR)
				{
					// 文件夹，递归调用扫描文件
					EnumNoUploadFiles(str.c_str(), nMin, nCount);
				}
				else
				{
					std::string::size_type pos = 0;
					// 过滤掉文件名后缀少于4字节的文件
					if ((pos = str.find_last_of('.', str.length())) < 4)
					{
						continue;
					}

					// 统计以: "_ok.txt" 的文件个数
					if (0 == str.compare(pos - 3, 7, kExeNameOk))
					{
						if (time(NULL) - st.st_mtime > nMin * 60)
						{
							++nCount;
						}
					}
				}
			}
		}
		closedir(pDir);
	}
}

int MessageFilesImpl::EnumNoUploadFiles(int nDaySpan, int nMin)
{
	int count = 0;
	time_t seconds = time(NULL) - nDaySpan * 24 * 3600;
	struct tm tm_time;

	// 使用localtime_r替换掉 localtime函数，线程安全
	localtime_r(&seconds, &tm_time);
	char buffer[16] = { 0 };
	snprintf(buffer, sizeof buffer, "%04d/%02d/%02d",
	         tm_time.tm_year + 1900, tm_time.tm_mon + 1, tm_time.tm_mday);

	for (auto it = details_.begin(); it != details_.end(); it++)
	{
		std::string pathTmp = szMainPath_;
		pathTmp.append(MSGFILES_FOLDER_NAME).append("/");
		pathTmp.append(szPtCode_).append("/");
		pathTmp.append(it->first).append("/");
		pathTmp.append(std::to_string(gateNo_)).append("/");
		pathTmp.append(buffer);

		EnumNoUploadFiles(pathTmp.c_str(), nMin, count);
	}

	return count;
}

void MessageFilesImpl::CloseAll()
{
	std::lock_guard<std::mutex> lock(mutex_);
	for (auto it = details_.begin(); it != details_.end(); ++it)
	{
		if (it->second)
		{
			it->second->Close();
		}
	}
}

} // end namespace MSGFILES
