#include <string.h>
#include <algorithm>
#include <vector>
#include <stdarg.h>
#include <mwnet_mt/crypto/mbedtls/base64.h>
#include <mwnet_mt/crypto/mbedtls/md5.h>
#include <mwnet_mt/crypto/mbedtls/sha256.h>

#include "MWStringUtil.h"

namespace MWSTRINGUTIL
{

void StringUtil::Split(const std::string& str,
                          const std::string& delim,
                          std::vector<std::string>* result)
{
	if (str.empty())
	{
		return;
	}

	if (delim[0] == '\0')
	{
		result->push_back(str);
		return;
	}

	size_t delimLength = delim.length();

	for (std::string::size_type beginIndex = 0; beginIndex < str.size();)
	{
		std::string::size_type endIndex = str.find(delim, beginIndex);
		if (endIndex == std::string::npos)
		{
			result->push_back(str.substr(beginIndex));
			return;
		}
		if (endIndex > beginIndex)
		{
			result->push_back(str.substr(beginIndex, (endIndex - beginIndex)));
		}

		beginIndex = endIndex + delimLength;
	}
}

bool StringUtil::StartsWith(const std::string& str, const std::string& prefix)
{
	if (prefix.length() > str.length())
	{
		return false;
	}
	if (memcmp(str.c_str(), prefix.c_str(), prefix.length()) == 0)
	{
		return true;
	}

	return false;
}

bool StringUtil::EndsWith(const std::string& str, const std::string& suffix)
{
	if (suffix.length() > str.length())
	{
		return false;
	}

	return (str.substr(str.length() - suffix.length()) == suffix);
}

std::string& StringUtil::Ltrim(std::string& str)
{
	std::string::iterator it =
	    find_if(str.begin(), str.end(), std::not1(std::ptr_fun(::isspace)));
	str.erase(str.begin(), it);
	return str;
}

std::string& StringUtil::Rtrim(std::string& str)
{
	std::string::reverse_iterator it =
	    find_if(str.rbegin(), str.rend(), std::not1(std::ptr_fun(::isspace)));
	str.erase(it.base(), str.end());
	return str;
}

std::string& StringUtil::Trim(std::string& str)
{
	return Rtrim(Ltrim(str));
}

std::string& StringUtil::TrimRight(std::string& str)
{
	return Rtrim(str);
}

std::string& StringUtil::TrimLeft(std::string& str)
{
	return Ltrim(str);
}

std::string& StringUtil::TrimRight(std::string& src, const std::string& s)
{
	StripSuffix(&src, s);
	return src;
}

std::string& StringUtil::TrimLeft(std::string& src, const std::string& s)
{
	StripPrefix(&src, s);
	return src;
}

void StringUtil::Trim(std::vector<std::string>* strList)
{
	if (nullptr == strList)
	{
		return;
	}

	std::vector<std::string>::iterator it;
	for (it = strList->begin(); it != strList->end(); ++it)
	{
		*it = Trim(*it);
	}
}

void StringUtil::StringReplace(const std::string& oldStr,
                                  const std::string& newStr,
                                  std::string* str)
{
	std::string::size_type pos = 0;
	std::string::size_type a = oldStr.size();
	std::string::size_type b = newStr.size();
	while ((pos = str->find(oldStr, pos)) != std::string::npos)
	{
		str->replace(pos, a, newStr);
		pos += b;
	}
}

void StringUtil::Replace(const std::string& oldStr, const std::string& newStr, std::string& str)
{
	StringReplace(oldStr, newStr, &str);
}


static const char ENCODECHARS[1024] = {
	3, '%', '0', '0', 3, '%', '0', '1', 3, '%', '0', '2', 3, '%', '0', '3',
	3, '%', '0', '4', 3, '%', '0', '5', 3, '%', '0', '6', 3, '%', '0', '7',
	3, '%', '0', '8', 3, '%', '0', '9', 3, '%', '0', 'A', 3, '%', '0', 'B',
	3, '%', '0', 'C', 3, '%', '0', 'D', 3, '%', '0', 'E', 3, '%', '0', 'F',
	3, '%', '1', '0', 3, '%', '1', '1', 3, '%', '1', '2', 3, '%', '1', '3',
	3, '%', '1', '4', 3, '%', '1', '5', 3, '%', '1', '6', 3, '%', '1', '7',
	3, '%', '1', '8', 3, '%', '1', '9', 3, '%', '1', 'A', 3, '%', '1', 'B',
	3, '%', '1', 'C', 3, '%', '1', 'D', 3, '%', '1', 'E', 3, '%', '1', 'F',
	1, '+', '2', '0', 3, '%', '2', '1', 3, '%', '2', '2', 3, '%', '2', '3',
	3, '%', '2', '4', 3, '%', '2', '5', 3, '%', '2', '6', 3, '%', '2', '7',
	3, '%', '2', '8', 3, '%', '2', '9', 3, '%', '2', 'A', 3, '%', '2', 'B',
	3, '%', '2', 'C', 1, '-', '2', 'D', 1, '.', '2', 'E', 3, '%', '2', 'F',
	1, '0', '3', '0', 1, '1', '3', '1', 1, '2', '3', '2', 1, '3', '3', '3',
	1, '4', '3', '4', 1, '5', '3', '5', 1, '6', '3', '6', 1, '7', '3', '7',
	1, '8', '3', '8', 1, '9', '3', '9', 3, '%', '3', 'A', 3, '%', '3', 'B',
	3, '%', '3', 'C', 3, '%', '3', 'D', 3, '%', '3', 'E', 3, '%', '3', 'F',
	3, '%', '4', '0', 1, 'A', '4', '1', 1, 'B', '4', '2', 1, 'C', '4', '3',
	1, 'D', '4', '4', 1, 'E', '4', '5', 1, 'F', '4', '6', 1, 'G', '4', '7',
	1, 'H', '4', '8', 1, 'I', '4', '9', 1, 'J', '4', 'A', 1, 'K', '4', 'B',
	1, 'L', '4', 'C', 1, 'M', '4', 'D', 1, 'N', '4', 'E', 1, 'O', '4', 'F',
	1, 'P', '5', '0', 1, 'Q', '5', '1', 1, 'R', '5', '2', 1, 'S', '5', '3',
	1, 'T', '5', '4', 1, 'U', '5', '5', 1, 'V', '5', '6', 1, 'W', '5', '7',
	1, 'X', '5', '8', 1, 'Y', '5', '9', 1, 'Z', '5', 'A', 3, '%', '5', 'B',
	3, '%', '5', 'C', 3, '%', '5', 'D', 3, '%', '5', 'E', 1, '_', '5', 'F',
	3, '%', '6', '0', 1, 'a', '6', '1', 1, 'b', '6', '2', 1, 'c', '6', '3',
	1, 'd', '6', '4', 1, 'e', '6', '5', 1, 'f', '6', '6', 1, 'g', '6', '7',
	1, 'h', '6', '8', 1, 'i', '6', '9', 1, 'j', '6', 'A', 1, 'k', '6', 'B',
	1, 'l', '6', 'C', 1, 'm', '6', 'D', 1, 'n', '6', 'E', 1, 'o', '6', 'F',
	1, 'p', '7', '0', 1, 'q', '7', '1', 1, 'r', '7', '2', 1, 's', '7', '3',
	1, 't', '7', '4', 1, 'u', '7', '5', 1, 'v', '7', '6', 1, 'w', '7', '7',
	1, 'x', '7', '8', 1, 'y', '7', '9', 1, 'z', '7', 'A', 3, '%', '7', 'B',
	3, '%', '7', 'C', 3, '%', '7', 'D', 1, '~', '7', 'E', 3, '%', '7', 'F',
	3, '%', '8', '0', 3, '%', '8', '1', 3, '%', '8', '2', 3, '%', '8', '3',
	3, '%', '8', '4', 3, '%', '8', '5', 3, '%', '8', '6', 3, '%', '8', '7',
	3, '%', '8', '8', 3, '%', '8', '9', 3, '%', '8', 'A', 3, '%', '8', 'B',
	3, '%', '8', 'C', 3, '%', '8', 'D', 3, '%', '8', 'E', 3, '%', '8', 'F',
	3, '%', '9', '0', 3, '%', '9', '1', 3, '%', '9', '2', 3, '%', '9', '3',
	3, '%', '9', '4', 3, '%', '9', '5', 3, '%', '9', '6', 3, '%', '9', '7',
	3, '%', '9', '8', 3, '%', '9', '9', 3, '%', '9', 'A', 3, '%', '9', 'B',
	3, '%', '9', 'C', 3, '%', '9', 'D', 3, '%', '9', 'E', 3, '%', '9', 'F',
	3, '%', 'A', '0', 3, '%', 'A', '1', 3, '%', 'A', '2', 3, '%', 'A', '3',
	3, '%', 'A', '4', 3, '%', 'A', '5', 3, '%', 'A', '6', 3, '%', 'A', '7',
	3, '%', 'A', '8', 3, '%', 'A', '9', 3, '%', 'A', 'A', 3, '%', 'A', 'B',
	3, '%', 'A', 'C', 3, '%', 'A', 'D', 3, '%', 'A', 'E', 3, '%', 'A', 'F',
	3, '%', 'B', '0', 3, '%', 'B', '1', 3, '%', 'B', '2', 3, '%', 'B', '3',
	3, '%', 'B', '4', 3, '%', 'B', '5', 3, '%', 'B', '6', 3, '%', 'B', '7',
	3, '%', 'B', '8', 3, '%', 'B', '9', 3, '%', 'B', 'A', 3, '%', 'B', 'B',
	3, '%', 'B', 'C', 3, '%', 'B', 'D', 3, '%', 'B', 'E', 3, '%', 'B', 'F',
	3, '%', 'C', '0', 3, '%', 'C', '1', 3, '%', 'C', '2', 3, '%', 'C', '3',
	3, '%', 'C', '4', 3, '%', 'C', '5', 3, '%', 'C', '6', 3, '%', 'C', '7',
	3, '%', 'C', '8', 3, '%', 'C', '9', 3, '%', 'C', 'A', 3, '%', 'C', 'B',
	3, '%', 'C', 'C', 3, '%', 'C', 'D', 3, '%', 'C', 'E', 3, '%', 'C', 'F',
	3, '%', 'D', '0', 3, '%', 'D', '1', 3, '%', 'D', '2', 3, '%', 'D', '3',
	3, '%', 'D', '4', 3, '%', 'D', '5', 3, '%', 'D', '6', 3, '%', 'D', '7',
	3, '%', 'D', '8', 3, '%', 'D', '9', 3, '%', 'D', 'A', 3, '%', 'D', 'B',
	3, '%', 'D', 'C', 3, '%', 'D', 'D', 3, '%', 'D', 'E', 3, '%', 'D', 'F',
	3, '%', 'E', '0', 3, '%', 'E', '1', 3, '%', 'E', '2', 3, '%', 'E', '3',
	3, '%', 'E', '4', 3, '%', 'E', '5', 3, '%', 'E', '6', 3, '%', 'E', '7',
	3, '%', 'E', '8', 3, '%', 'E', '9', 3, '%', 'E', 'A', 3, '%', 'E', 'B',
	3, '%', 'E', 'C', 3, '%', 'E', 'D', 3, '%', 'E', 'E', 3, '%', 'E', 'F',
	3, '%', 'F', '0', 3, '%', 'F', '1', 3, '%', 'F', '2', 3, '%', 'F', '3',
	3, '%', 'F', '4', 3, '%', 'F', '5', 3, '%', 'F', '6', 3, '%', 'F', '7',
	3, '%', 'F', '8', 3, '%', 'F', '9', 3, '%', 'F', 'A', 3, '%', 'F', 'B',
	3, '%', 'F', 'C', 3, '%', 'F', 'D', 3, '%', 'F', 'E', 3, '%', 'F', 'F',
};

void StringUtil::UrlEncode(const std::string& srcStr, std::string* dstStr)
{
	dstStr->clear();
	for (size_t i = 0; i < srcStr.length(); i++)
	{
		unsigned short offset = static_cast<unsigned short>(srcStr[i] * 4);
		dstStr->append((ENCODECHARS + offset + 1), ENCODECHARS[offset]);
	}
}

std::string& StringUtil::UrlEncode(const std::string& srcStr, std::string& dstStr)
{
	UrlEncode(srcStr,&dstStr);
	return dstStr;
}

std::string& StringUtil::UrlDecode(const std::string& srcStr, std::string& dstStr)
{
	UrlDecode(srcStr, &dstStr);
	return dstStr;
}

static const char HEX2DEC[256] = {
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	0,  1,  2,  3,  4,  5,  6,  7,  8,  9, -1, -1, -1, -1, -1, -1,
	-1, 10, 11, 12, 13, 14, 15, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, 10, 11, 12, 13, 14, 15, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
};

void StringUtil::UrlDecode(const std::string& srcStr, std::string* dstStr)
{
	dstStr->clear();
	const unsigned char* srcBegin = reinterpret_cast<const unsigned char*>(srcStr.data());
	const unsigned char* srcEnd = srcBegin + srcStr.length();
	const unsigned char* srcLast = srcEnd - 2;

	while (srcBegin < srcLast)
	{
		if ((*srcBegin) == '%')
		{
			char dec1, dec2;
			if (-1 != (dec1 = HEX2DEC[*(srcBegin + 1)])
			        && -1 != (dec2 = HEX2DEC[*(srcBegin + 2)]))
			{
				dstStr->append(1, (dec1 << 4) + dec2);
				srcBegin += 3;
				continue;
			}
		}
		else if ((*srcBegin) == '+')
		{
			dstStr->append(1, ' ');
			++srcBegin;
			continue;
		}
		dstStr->append(1, static_cast<char>(*srcBegin));
		++srcBegin;
	}

	while (srcBegin < srcEnd)
	{
		dstStr->append(1, static_cast<char>(*srcBegin));
		++srcBegin;
	}
}

void StringUtil::ToUpper(std::string* str)
{
	std::transform(str->begin(), str->end(), str->begin(), ::toupper);
}

void StringUtil::ToLower(std::string* str)
{
	std::transform(str->begin(), str->end(), str->begin(), ::tolower);
}

std::string& StringUtil::ToUpper(std::string& str)
{
	std::transform(str.begin(), str.end(), str.begin(), ::toupper);
	return str;
}

std::string& StringUtil::ToLower(std::string& str)
{
	std::transform(str.begin(), str.end(), str.begin(), ::tolower);
	return str;
}

bool StringUtil::StripSuffix(std::string* str, const std::string& suffix)
{
	if (str->length() >= suffix.length())
	{
		size_t suffixPos = str->length() - suffix.length();
		if (str->compare(suffixPos, std::string::npos, suffix) == 0)
		{
			str->resize(str->size() - suffix.size());
			return true;
		}
	}

	return false;
}

bool StringUtil::StripPrefix(std::string* str, const std::string& prefix)
{
	if (str->length() >= prefix.length())
	{
		if (str->substr(0, prefix.size()) == prefix)
		{
			*str = str->substr(prefix.size());
			return true;
		}
	}

	return false;
}

std::string& StringUtil::HexStringToBytes(const char* hexStr, size_t len, std::string& bytes)
{
	Hex2Bin(hexStr, &bytes);
	return bytes;
}

void StrToHex(const char* src, size_t srcLen, std::string& hexStr, bool upper)
{
	hexStr.clear();
	
	hexStr.reserve(2 * srcLen + 4);

	static char lowerHex[] = "0123456789abcdef";
	static char upperHex[] = "0123456789ABCDEF";
	if (upper)
	{
		for(size_t i = 0; i < srcLen; i++)
		{		
			char high = upperHex[static_cast<uint8_t>(src[i]) >> 4];
			char low  = upperHex[static_cast<uint8_t>(src[i]) & 0xf];	
			hexStr.append(1, high);
			hexStr.append(1, low);
		}
	}
	else 
	{
		for(size_t i = 0; i < srcLen; i++)
		{		
			char high = lowerHex[static_cast<uint8_t>(src[i]) >> 4];
			char low  = lowerHex[static_cast<uint8_t>(src[i]) & 0xf];	
			hexStr.append(1, high);
			hexStr.append(1, low);
		}
	}
}

std::string& StringUtil::BytesToHexString(const std::string& bytes, std::string& hexStr, bool upper)
{
	StrToHex(bytes.data(), bytes.size(), hexStr, upper); 	
	return hexStr;
}

std::string& StringUtil::BytesToHexString(const char* bytes, size_t len, std::string& hexStr, bool upper)
{
	StrToHex(bytes, len, hexStr, upper);
	return hexStr;
}

std::string& StringUtil::BytesToBase64String(const std::string& bytes, std::string& base64Str)
{
	return BytesToBase64String(bytes.data(), bytes.size(), base64Str);
}

std::string& StringUtil::ToMd5Bytes(const std::string& bytes, std::string& md5)
{
	return ToMd5Bytes(bytes.data(), bytes.size(), md5);
}
std::string& StringUtil::ToMd5HexString(const std::string& bytes, std::string& md5HexStr, bool upper)
{
	return ToMd5HexString(bytes.data(), bytes.size(), md5HexStr, upper);
}

std::string& StringUtil::ToSha256Bytes(const std::string& bytes, std::string& sha256)
{
	return ToSha256Bytes(bytes.data(), bytes.size(), sha256);
}
std::string& StringUtil::ToSha256HexString(const std::string& bytes, std::string& sha256HexStr, bool upper)
{
	return ToSha256HexString(bytes.data(), bytes.size(), sha256HexStr, upper);
}

std::string StringUtil::FormatString(const char *fmt, ...)
{
	/*
	{
		const int nReserved = 512;
		std::string strRet(nReserved, '\0');
		va_list marker;
		va_start(marker, fmt);
		int nNeeded = vsnprintf(const_cast<char* >(strRet.data()), nReserved+1, fmt, marker);
		va_end(marker);
		if (nNeeded > 0 && nNeeded <= nReserved)
		{
			strRet.resize(nNeeded);
		}
		else
		{
			goto FMT1;
		}
		return strRet;
	}
FMT1:	
	{
		va_list marker;
		va_start(marker, fmt);
		int nNeeded = vsnprintf(NULL, 0, fmt, marker);
		va_end(marker);
		if (nNeeded < 0) return "";
		va_start(marker, fmt);
		std::string strRet(nNeeded, '\0');
		int nSize = vsnprintf(const_cast<char* >(strRet.data()), nNeeded+1, fmt, marker);
		va_end(marker);
		if (nSize > 0 && nSize <= nNeeded)
		{
			strRet.resize(nSize);
		}
		else
		{
			strRet = "";
		}
		return strRet;
	}
	*/
	va_list marker;
	va_start(marker, fmt);
	int nNeeded = vsnprintf(NULL, 0, fmt, marker);
	va_end(marker);
	if (nNeeded < 0) return "";
	va_start(marker, fmt);
	std::string strRet(nNeeded, '\0');
	int nSize = vsnprintf(const_cast<char* >(strRet.data()), nNeeded+1, fmt, marker);
	va_end(marker);
	if (nSize > 0 && nSize <= nNeeded)
	{
		strRet.resize(nSize);
	}
	else
	{
		strRet = "";
	}
	return strRet;
}

std::string& StringUtil::FormatString(std::string& str, const char *fmt, ...)
{
	/*
	{
		const int nReserved = 512;
		str.resize(nReserved);
		va_list marker;
		va_start(marker, fmt);
		int nNeeded = vsnprintf(const_cast<char* >(str.data()), nReserved+1, fmt, marker);
		va_end(marker);
		if (nNeeded > 0 && nNeeded <= nReserved)
		{
			str.resize(nNeeded);
		}
		else
		{
			goto FMT1;
		}
		return str;
	}
FMT1:	
	{
		va_list marker;
		va_start(marker, fmt);
		int nNeeded = vsnprintf(NULL, 0, fmt, marker);
		va_end(marker);
		if (nNeeded <= 0) 
		{
			str = "";
			return str;
		}
		va_start(marker, fmt);
		str.resize(nNeeded);
		int nSize = vsnprintf(const_cast<char* >(str.data()), nNeeded+1, fmt, marker);
		va_end(marker);
		if (nSize > 0 && nSize <= nNeeded)
		{
			str.resize(nSize);
		}
		else
		{
			str = "";
		}
		return str;
	}
	*/
	va_list marker;
	va_start(marker, fmt);
	int nNeeded = vsnprintf(NULL, 0, fmt, marker);
	va_end(marker);
	if (nNeeded <= 0) 
	{
		str = "";
		return str;
	}
	va_start(marker, fmt);
	str.resize(nNeeded);
	int nSize = vsnprintf(const_cast<char* >(str.data()), nNeeded+1, fmt, marker);
	va_end(marker);
	if (nSize > 0 && nSize <= nNeeded)
	{
		str.resize(nSize);
	}
	else
	{
		str = "";
	}
	return str;
}

#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wold-style-cast"

bool StringUtil::Hex2Bin(const char* hexStr, std::string* binStr)
{
	if (nullptr == hexStr || nullptr == binStr)
	{
		return false;
	}

	binStr->clear();
	while (*hexStr != '\0')
	{
		if (hexStr[1] == '\0')
		{
			return false;
		}

		uint8_t high = static_cast<uint8_t>(hexStr[0]);
		uint8_t low  = static_cast<uint8_t>(hexStr[1]);
#define ASCII2DEC(c) \
		if (c >= '0' && c<= '9') c -= '0'; \
		else if (c >= 'A' && c <= 'F') c -= ('A' - 10); \
		else if (c >= 'a' && c <= 'f') c -= ('a' - 10); \
		else return false

		ASCII2DEC(high);
		ASCII2DEC(low);
		binStr->append(1, static_cast<char>((high << 4) + low));
		hexStr += 2;
	}

	return true;
}

bool StringUtil::Bin2Hex(const char* binStr, size_t binLen, std::string* hexStr)
{
	if (nullptr == binStr || nullptr == hexStr)
	{
		return false;
	}
	hexStr->clear();

	for (size_t i = 0; i < binLen; i++)
	{
		uint8_t high = (static_cast<uint8_t>(binStr[i]) >> 4);
		uint8_t low  = (static_cast<uint8_t>(binStr[i]) & 0xF);
#define DEC2ASCII(c) \
		if (c <= 9) c += '0'; \
		else c += ('A' - 10)

		DEC2ASCII(high);
		DEC2ASCII(low);
		hexStr->append(1, static_cast<char>(high));
		hexStr->append(1, static_cast<char>(low));
		// binStr += 1;
	}
	return true;
}

std::string& StringUtil::BytesToBase64String(const char* bytes, size_t len, std::string& base64Str)
{
	size_t destLen = len * 4 / 3 + 4;
	base64Str.clear();
	base64Str.resize(destLen);
	if (mbedtls_base64_encode((uint8_t*)(base64Str.data()), destLen, &destLen, (const uint8_t*)(bytes), len) != 0)
	{
		base64Str = "";	  
	}
	else
	{
		base64Str.resize(destLen);
	}
	
	return base64Str;
}
std::string& StringUtil::Base64StringToBytes(const char* base64Str, size_t len, std::string& bytes)
{
    size_t destLen = len * 3 / 4 + 4;
	bytes.clear();
    bytes.resize(destLen);
    
    if (mbedtls_base64_decode((uint8_t*)(bytes.data()), destLen, &destLen, (const uint8_t*)(base64Str), len) != 0)
    {
		bytes = "";   
	}
	else
	{
		bytes.resize(destLen);
	}

    return bytes;
}

std::string& StringUtil::ToMd5Bytes(const char* bytes, size_t len, std::string& md5)
{
	md5.clear();
	md5.resize(16);
	mbedtls_md5((const uint8_t*)bytes, len, (uint8_t*)md5.data());
	return md5;
}
std::string& StringUtil::ToMd5HexString(const char* bytes, size_t len, std::string& md5HexStr, bool upper)
{
	std::string md5 = "";
	ToMd5Bytes(bytes, len, md5);
	BytesToHexString(md5.data(), md5.size(), md5HexStr, upper);
	return md5HexStr;
}

std::string& StringUtil::ToSha256Bytes(const char* bytes, size_t len, std::string& sha256)
{
	sha256.clear();
	sha256.resize(32);
	mbedtls_sha256((const uint8_t*)bytes, len, (uint8_t*)sha256.data(), 0);
	return sha256;
}
std::string& StringUtil::ToSha256HexString(const char* bytes, size_t len, std::string& sha256HexStr, bool upper)
{
	std::string sha256 = "";
	ToSha256Bytes(bytes, len, sha256);
	BytesToHexString(sha256.data(), sha256.size(), sha256HexStr, upper);
	return sha256HexStr;
}

#pragma GCC diagnostic warning "-Wconversion"
#pragma GCC diagnostic error "-Wold-style-cast"

} // end namespace common
