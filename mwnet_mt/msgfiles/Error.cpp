#include "Error.h"
#include <unordered_map>

namespace common
{

// 避免全局变量构造、析构顺序问题
static std::unordered_map<int32_t, const char*>* gErrorStringMap = nullptr;
struct ErrorInfo
{
	ErrorInfo()
	{
		gErrorStringMap = &errorInfo_;
	}
	~ErrorInfo()
	{
		gErrorStringMap = nullptr;
	}
	std::unordered_map<int32_t, const char* > errorInfo_;
};

const char* GetErrorString(int32_t errorCode)
{
	if (0 == errorCode) return "no error";

	if (!gErrorStringMap)
	{
		return "error module not inited.";
	}

	std::unordered_map<int32_t, const char*>::iterator it = gErrorStringMap->find(errorCode);
	if (it != gErrorStringMap->end())
	{
		return it->second;
	}

	return "unreister error description";
}

void SetErrorString(int32_t errorCode, const char* errorString)
{
	static ErrorInfo sErrorInfo;
	(*gErrorStringMap)[errorCode] = errorString;
}

} // end namespace common
