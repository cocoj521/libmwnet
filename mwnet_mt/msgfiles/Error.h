#pragma once

#include <stdio.h>
#include <string>

/**
 * 错误输出原则：
 * 1> common库,功能聚焦api明确，返回错误码，记录错误信息供用户获取,不输出日志
 * 2> framework,关键流程错误必须输出日志，注意比喵重复输出
 * 3> 对于暴露给用户的接口，返回错误应该是最根本的错误原因，避免返回中间错误信息
 */

/// 格式化输出log信息到buff
#define LOG_MESSAGE(buff, buffLen, fmt, ...) \
	snprintf((buff), (buffLen), "(%s:%d)(%s)" fmt, \
	__FILE__, __LINE__, __FUNCTION__, ##__VA_ARGS__)

/// 记录最后错误信息，内部使用，要求buff名字为 lastError_
#define _LOG_LAST_ERROR(fmt, ...) \
	LOG_MESSAGE((lastError_), (sizeof(lastError_)), fmt, ##__VA_ARGS__)

namespace common
{

enum ERROR_CODE_BASE
{
	NO_ERROR                  = 0,
	RPC_ERROR_CODE_BASE       = -1000,
	SESSION_ERROR_CODE_BASE   = -2000,
	MESSAGE_ERROR_CODE_BASE   = -3000,
	NAMING_ERROR_CODE_BASE    = -4000,
	ROUTER_ERROR_CODE_BASE    = -5000,
	TIMER_ERROR_CODE_BASE     = -6000,
	PIPE_ERROR_CODE_BASE      = -7000,
	COROUTINE_ERROR_CODE_BASE = -8000,
	CHANNEL_ERROR_CODE_BASE   = -9000,
	PROCESSOR_ERROR_CODE_BASE = -10000,
	USER_ERROR_CODE_BASE      = -100000,
};

/// 获取错误的描述
const char* GetErrorString(int32_t errorCode);

/// 设置错误描述
void SetErrorString(int32_t errorCode, const char* errorString);

} // end namespace common
