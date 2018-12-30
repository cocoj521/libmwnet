#ifndef __MESSAGEFILES_H__
#define __MESSAGEFILES_H__

#include <string>

#define MSGFILES_FOLDER_NAME	"MsgFiles"
#define FILE_EXNAME			 	".txt"

namespace MSGFILES
{

class MessageFiles
{
public:
	MessageFiles() = default;
	virtual ~MessageFiles() = default;

public:
	virtual bool InitParams(const std::string& szPtCode, int ptid, int gateNo) = 0;
	virtual void SetParams(int nSec = 180/*second*/, int maxFileSize = 20/*M*/) = 0;
	virtual int  InitMsgDetailsFile(const std::string& szPtCode = "ptcode",
	                                   int ptCode = 0,
	                                   int gateNo = 0,
	                                   const std::string& fileType = "filetype",
	                                   int maxFileSize = 20,
	                                   int genFreq = 180) = 0;

	virtual bool bExistFile(const std::string& fileType) = 0;
	virtual int  WriteFile(const std::string& fileType, const std::string& data) = 0;

	virtual void SwitchMessageFiles() = 0;
	virtual int  GetLongTimeNoUploadFilesCnt(int min = 10) = 0;
	virtual void CloseAll() = 0;
};

class MessageFilesFactory
{
public:
	static MessageFiles* New();
	static void Destroy(MessageFiles* files);
};

} // end namespace MSGFILES

#endif // __MESSAGEFILES_H__
