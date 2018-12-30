#ifndef __MESSAGE_FILES_IMPL_H__
#define __MESSAGE_FILES_IMPL_H__

#include <string>
#include <vector>
#include <map>

#include "FileMgr.h"
#include "MessageFiles.h"

#include <mutex>

namespace MSGFILES
{

typedef std::map<std::string, FileMgr*> FileMgrMap;

class MessageFilesImpl : public MessageFiles
{
public:
	MessageFilesImpl();
	virtual ~MessageFilesImpl() = default;

public:
	virtual bool InitParams(const std::string& szPtCode, int ptid, int gateNo);
	virtual void SetParams(int seconds = 180/*second*/, int maxFileSize = 20/*M*/);
	virtual int  InitMsgDetailsFile(const std::string& szPtCode = "UNKNOWN",
	                                   int ptCode = 0,
	                                   int gateNo = 0,
	                                   const std::string& fileType = "MT_RVOK",
	                                   int maxFileSize = 20,
	                                   int genFreq = 180);

	virtual bool bExistFile(const std::string& fileType);
	virtual int  WriteFile(const std::string& fileType, const std::string& data);
	virtual void SwitchMessageFiles();
	virtual int  GetLongTimeNoUploadFilesCnt(int min = 10);
	virtual void CloseAll();

private:
	// 扫描没有被重命名的文件，将其重命名为_ok.txt
	void ScanfAndRenameTmpFile();

	// 扫描tmp文件
	void EnumTmpFile(const char* szFolderName, std::vector<std::string>& vFileName);

	// 扫描没有被及时上传的文件
	void EnumNoUploadFiles(const char* szFolderName, int nMin, int& nCount);

	// 扫描距当前若干天前的未上传的文件
	int  EnumNoUploadFiles(int nDaySpan, int nMin);

private:
	bool bDldFileInit_;
	std::string szMainPath_;
	int gateNo_;
	std::string szPtCode_;
	int ptCode_;

	std::mutex mutex_;
	FileMgrMap details_;
};

} // end namespace MSGFILES

#endif // __MESSAGE_FILES_IMPL_H__
