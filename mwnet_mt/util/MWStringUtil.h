#ifndef MW_STRINGUTIL_H
#define MW_STRINGUTIL_H

#include <string>
#include <iostream> 

namespace MWSTRINGUTIL
{

class StringUtil
{
public:
	// ���ָ������
	static void Split(const std::string& str, const std::string& delim, std::vector<std::string>* result);

	// �ж��ַ���str�Ƿ�����prefix��ͷ
	static bool StartsWith(const std::string& str, const std::string& prefix);
	static bool EndsWith(const std::string& str, const std::string& suffix);

	// ȥ��/��trim�ո�
	static std::string& TrimRight(std::string& str);
	static std::string& TrimLeft(std::string& str);
	static std::string& Ltrim(std::string& str); // NOLINT
	static std::string& Rtrim(std::string& str); // NOLINT
	
	// ���ҿո�ȥ��
	static std::string& Trim(std::string& str);  // NOLINT

	// ȥ�������е��ַ������ҵĿո�
	static void Trim(std::vector<std::string>* strList);

	// ����trim�ַ���
	static std::string& TrimRight(std::string& src, const std::string& s);
	static std::string& TrimLeft(std::string& src, const std::string& s);
	
	// �Ӵ��滻
	static void StringReplace(const std::string& oldStr, const std::string& newStr, std::string* str);
	static void Replace(const std::string& oldStr, const std::string& newStr, std::string& str);
	
	// urlencode and decode
	static std::string& UrlEncode(const std::string& srcStr, std::string& dstStr);
	static std::string& UrlDecode(const std::string& srcStr, std::string& dstStr);
	static void UrlEncode(const std::string& srcStr, std::string* dstStr);
	static void UrlDecode(const std::string& srcStr, std::string* dstStr);

	// ��Сдת��
	static std::string& ToUpper(std::string& str);
	static std::string& ToLower(std::string& str);
	static void ToUpper(std::string* str);
	static void ToLower(std::string* str);
	
	// ȥͷȥβ
	static bool StripSuffix(std::string* str, const std::string& suffix);
	static bool StripPrefix(std::string* str, const std::string& prefix);

	// ��16�����ַ�����ԭ
	static std::string& HexStringToBytes(const char* hexStr, size_t len, std::string& bytes);	
	static bool Hex2Bin(const char* hexStr, std::string* binStr);
	
	// ת����16�����ַ���
	static std::string& BytesToHexString(const std::string& bytes, std::string& hexStr, bool upper=false); // Ĭ��Сд
	static std::string& BytesToHexString(const char* bytes, size_t len, std::string& hexStr, bool upper=false); // Ĭ��Сд
	static std::string  BytesToHexString(const char* bytes, size_t len, bool upper = false); // Ĭ��Сд
	static bool Bin2Hex(const char* binStr, size_t binLen, std::string* hexStr);	// �̶���д

	// base64�����
	static std::string& BytesToBase64String(const std::string& bytes, std::string& base64Str);
	static std::string& BytesToBase64String(const char* bytes, size_t len, std::string& base64Str);
	static std::string& Base64StringToBytes(const char* base64Str, size_t len, std::string& bytes);	

	// ����MD5
	static std::string& ToMd5Bytes(const char* bytes, size_t len, std::string& md5);
	static std::string& ToMd5Bytes(const std::string& bytes, std::string& md5);
	static std::string& ToMd5HexString(const std::string& bytes, std::string& md5HexStr, bool upper=false); // Ĭ��Сд
	static std::string& ToMd5HexString(const char* bytes, size_t len, std::string& md5HexStr, bool upper=false); // Ĭ��Сд

	// ����SHA256
	static std::string& ToSha256Bytes(const char* bytes, size_t len, std::string& sha256);
	static std::string& ToSha256Bytes(const std::string& bytes, std::string& sha256);
	static std::string& ToSha256HexString(const std::string& bytes, std::string& sha256HexStr, bool upper=false); // Ĭ��Сд
	static std::string& ToSha256HexString(const char* bytes, size_t len, std::string& sha256HexStr, bool upper=false); // Ĭ��Сд

	// ����SHA1
	static std::string& ToSha1Bytes(const char* bytes, size_t len, std::string& sha1);
	static std::string& ToSha1Bytes(const std::string& bytes, std::string& sha1);
	static std::string& ToSha1HexString(const std::string& bytes, std::string& sha1HexStr, bool upper = false); // Ĭ��Сд
	static std::string& ToSha1HexString(const char* bytes, size_t len, std::string& sha1HexStr, bool upper = false); // Ĭ��Сд

	// same as std::to_string
	template<typename T>
	static std::string  ToString(const T& in)
	{
        return std::to_string(in);  
	}

	// ��ʽ���ַ��� like CString Format 
	static std::string  FormatString(const char *fmt, ...);
	static std::string& FormatString(std::string& str, const char *fmt, ...);
	
};

} // end namespace
#endif
