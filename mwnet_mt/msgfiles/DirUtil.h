#pragma once

#include <string>

namespace common
{

class DirUtil
{
public:
	/**
	 * [MakeDir 创建目录不存在时创建(同mkdir)]
	 * @author dengyu 2017-12-07
	 * @param  path [要创建的目录]
	 * @return      [0成功 非0失败]
	 */
	static int MakeDir(const std::string& path);

	/**
	 * [MakeDirP 递归创建目录(同mkdir -p)]
	 * @author dengyu 2017-12-07
	 * @param  path [要创建的目录]
	 * @return      [0成功 非0失败]
	 */
	static int MakeDirP(const std::string& path);
	
	static std::string GetExePath(); 

	static int mkdirs(const char *muldir);
	
	static const char* GetLastError()
	{
		return lastError_;
	}
	
private:
	static char lastError_[256];
};

} // end namespace common
