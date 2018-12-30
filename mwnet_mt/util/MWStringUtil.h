#ifndef MW_STRINGUTIL_H
#define MW_STRINGUTIL_H

#include <string>
#include <iostream> 

namespace MWSTRINGUTIL
{

class StringUtil
{
public:
	// 按分隔符拆分
	static void Split(const std::string& str, const std::string& delim, std::vector<std::string>* result);

	// 判断字符串str是否是以prefix开头
	static bool StartsWith(const std::string& str, const std::string& prefix);
	static bool EndsWith(const std::string& str, const std::string& suffix);

	// 去左/右trim空格
	static std::string& TrimRight(std::string& str);
	static std::string& TrimLeft(std::string& str);
	static std::string& Ltrim(std::string& str); // NOLINT
	static std::string& Rtrim(std::string& str); // NOLINT
	
	// 左右空格都去掉
	static std::string& Trim(std::string& str);  // NOLINT

	// 去除数组中的字符串左右的空格
	static void Trim(std::vector<std::string>* strList);

	// 左右trim字符串
	static std::string& TrimRight(std::string& src, const std::string& s);
	static std::string& TrimLeft(std::string& src, const std::string& s);
	
	// 子串替换
	static void StringReplace(const std::string& oldStr, const std::string& newStr, std::string* str);
	static void Replace(const std::string& oldStr, const std::string& newStr, std::string& str);
	
	// urlencode and decode
	static std::string& UrlEncode(const std::string& srcStr, std::string& dstStr);
	static std::string& UrlDecode(const std::string& srcStr, std::string& dstStr);
	static void UrlEncode(const std::string& srcStr, std::string* dstStr);
	static void UrlDecode(const std::string& srcStr, std::string* dstStr);

	// 大小写转换
	static std::string& ToUpper(std::string& str);
	static std::string& ToLower(std::string& str);
	static void ToUpper(std::string* str);
	static void ToLower(std::string* str);
	
	// 去头去尾
	static bool StripSuffix(std::string* str, const std::string& suffix);
	static bool StripPrefix(std::string* str, const std::string& prefix);

	// 将16进制字符串还原
	static std::string& HexStringToBytes(const char* hexStr, size_t len, std::string& bytes);	
	static bool Hex2Bin(const char* hexStr, std::string* binStr);
	
	// 转换成16进制字符串
	static std::string& BytesToHexString(const std::string& bytes, std::string& hexStr, bool upper=false); // 默认小写
	static std::string& BytesToHexString(const char* bytes, size_t len, std::string& hexStr, bool upper=false); // 默认小写
	static const char*  BytesToHexString(const std::string& bytes, bool upper=false); // 默认小写
	static const char*  BytesToHexString(const char* bytes, size_t len, bool upper=false); // 默认小写
	static bool Bin2Hex(const char* binStr, size_t binLen, std::string* hexStr);	// 固定大写

	// base64编解码
	static std::string& BytesToBase64String(const std::string& bytes, std::string& base64Str);
	static std::string& BytesToBase64String(const char* bytes, size_t len, std::string& base64Str);
	static const char*  BytesToBase64String(const std::string& bytes);
	static const char*  BytesToBase64String(const char* bytes, size_t len);
	static std::string& Base64StringToBytes(const char* base64Str, size_t len, std::string& bytes);	

	// 生成MD5
	static std::string& ToMd5Bytes(const char* bytes, size_t len, std::string& md5);
	static std::string& ToMd5Bytes(const std::string& bytes, std::string& md5);
	static std::string& ToMd5HexString(const std::string& bytes, std::string& md5HexStr, bool upper=false); // 默认小写
	static std::string& ToMd5HexString(const char* bytes, size_t len, std::string& md5HexStr, bool upper=false); // 默认小写
	static const char*  ToMd5HexString(const std::string& bytes, bool upper=false); // 默认小写
	static const char*  ToMd5HexString(const char* bytes, size_t len, bool upper=false); // 默认小写

	// same as std::to_string
	template<typename T>
	static std::string  ToString(const T& in)
	{
        return std::to_string(in);  
	}

	// 格式化字符串 like CString Format 
	static std::string  FormatString(const char *fmt, ...);
	static std::string& FormatString(std::string& str, const char *fmt, ...);
	
};

} // end namespace
#endif
