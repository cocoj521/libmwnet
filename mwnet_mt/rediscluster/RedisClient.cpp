#include "RedisClient.h"
#include <iostream>
#include <common/Monitor.h>

using namespace rediscluster;

RedisClient::RedisClient(RedisClusterNode* manager)
	: connStatus_(false),
	  port_(7000),
	  timeout_(120),
	  ipStr_("127.0.0.1"),
	  password_(""),
	  context_(NULL),
	  manager_(manager)
{
}

RedisClient::~RedisClient()
{
	release();
}

bool RedisClient::init(const std::string& ipStr, int port, int timeout, const std::string& password)
{
	ipStr_ = ipStr;
	port_  = port;
	timeout_ = timeout;
	password_ = password;
	return true;
}

redisContext* RedisClient::connectWithTimeout()
{
	struct timeval timeoutVal;
	timeoutVal.tv_sec  = timeout_;
	timeoutVal.tv_usec = 0;

	context_ = redisConnectWithTimeout(ipStr_.c_str(), port_, timeoutVal);

	if (context_ && context_->err)
	{
		setLastError(context_->errstr, strlen(context_->errstr));
		redisFree(context_);
		context_ = NULL;
	}

	return context_;
}

bool RedisClient::connect()
{
	if (context_ != NULL)
	{
		release();
		context_ = NULL;
	}

	context_ = connectWithTimeout();
	if (NULL == context_)
	{
		connStatus_ = false;
	}
	else
	{
		connStatus_ = auth() && ping();
	}
	return connStatus_;
}


bool RedisClient::reConnect()
{
	// 先关闭链接再连接
	release();
	return connect();
}

bool RedisClient::auth()
{
	// 未设置密码校验直接返回true
	if ("" == password_ || password_.empty())
	{
		connStatus_ = true;
		return true;
	}

	redisReply* reply = static_cast<redisReply* >(redisCommand(context_, "AUTH %s", password_.c_str()));
	if (reply == NULL)
	{
		setLastError("reply is null", strlen("reply is null"));
		return false;
	}
	
	connStatus_ = ((reply->str) && (strcasecmp(reply->str, "OK") == 0));
	if ((REDIS_REPLY_ERROR == reply->type) && reply->str)
	{
		setLastError(reply->str, reply->len);
	}

	freeReplyObject(reply);
	return connStatus_;
}

void RedisClient::release()
{
	if (NULL != context_)
	{
		redisFree(context_);
		context_ = NULL;
		connStatus_ = false;
	}
}

void RedisClient::recycle()
{
	if (manager_)
	{
		if (connStatus_)
		{
			// 正常回收连接
			manager_->releaseRedisClient(this);
		}
		else
		{
			printf("connect host=%s port=%d find error=%s will remove client", ipStr_.c_str(), port_, lastError_.c_str());
			// 发生了错误后,需要清除这个连接
			manager_->removeRedisClient(this);
		}

	}
}

bool RedisClient::ping()
{
	if (context_ == NULL) return false;
	redisReply* reply = static_cast<redisReply* >(redisCommand(context_, "PING"));
	connStatus_ = ((NULL != reply) && (reply->str) && (strcasecmp(reply->str, "PONG") == 0));
	
	if(NULL == reply) return false;
	
	if (reply)
	{
		if ((reply->type == REDIS_REPLY_ERROR) && (reply->str))
		{
			setLastError(reply->str, reply->len);
		}

		freeReplyObject(reply);
	}
	return connStatus_;
}

//执行lua脚本
bool RedisClient::command(std::string& result, const char* cmd, ...)
{
	if (NULL == cmd)
	{
		return false;
	}

	if (context_ == NULL)
	{
		return false;
	}

	va_list args;
	va_start(args, cmd);
	redisReply* reply = static_cast<redisReply*>(redisvCommand(context_, cmd, args));
	va_end(args);

	bool bRet = false;
	if (checkReply(reply))
	{
		result.assign(reply->str, reply->len);
		bRet = true;
	}
	else
	{
		if (!reply)
		{
			// 发生错误
			setLastError("系统错误", strlen("系统错误"));
		}
		else if (reply->type == REDIS_REPLY_NIL)
		{
			// 这不是一种错误
			// setLastError("nil", strlen("nil"));
		}
		else
		{
			// 发生错误
			setLastError(reply->str, reply->len);
		}
	}

	if (NULL != reply)
	{
		freeReplyObject(reply);
	}

	return bRet;
	//Examples: eval "return KEYS[1]" 1 key1 value1 key2 value2 ... keyN valueN
	//return commandString(result, "EVAL %s %s", luaScript.c_str(), parameters.c_str());
}

bool RedisClient::command(HashMapDataItem& dataItemMap, const char* cmd, ...)
{
	if (NULL == cmd)
	{
		return false;
	}

	if (context_ == NULL)
	{
		return false;
	}

	va_list args;
	va_start(args, cmd);
	redisReply* reply = static_cast<redisReply*>(redisvCommand(context_, cmd, args));
	va_end(args);

	bool bRet = false;
	if (checkReply(reply))
	{
		for (size_t i = 0; i < reply->elements; i += 2)
		{
			std::string key;
			key.assign(reply->element[i]->str, reply->element[i]->len);

			DataItem item;
			item.type = reply->element[i + 1]->type;
			item.value.assign(reply->element[i + 1]->str, reply->element[i + 1]->len);

			dataItemMap.insert(std::make_pair(key, item));
		}
		bRet = true;
	}
	else
	{
		if (!reply)
		{
			// 发生错误
			setLastError("系统错误", strlen("系统错误"));
		}
		else if (reply->type == REDIS_REPLY_NIL)
		{
			// 返回值为空
			setLastError("nil", strlen("nil"));
		}
		else
		{
			// 发生错误
			setLastError(reply->str, reply->len);
		}
	}

	if (NULL != reply)
	{
		freeReplyObject(reply);
	}

	return bRet;
	//Examples: eval "return KEYS[1]" 1 key1 value1 key2 value2 ... keyN valueN
	//return commandString(result, "EVAL %s %s", luaScript.c_str(), parameters.c_str());
}



bool RedisClient::commandLuaList(std::vector<std::string>& vValue, const char* cmd, ...)
{
    if (NULL == cmd)
    {
        return false;
    }

    if (context_ == NULL)
    {
        return false;
    }

    va_list args;
    va_start(args, cmd);
    redisReply* reply = static_cast<redisReply*>(redisvCommand(context_, cmd, args));
    va_end(args);

    bool bRet = false;
    if (checkReply(reply))
    {
        for (size_t i = 0; i < reply->elements; i++)
        {
            vValue.push_back(std::string(reply->element[i]->str, reply->element[i]->len));
        }
        bRet = true;
    }
    else
    {
        if (!reply)
        {
            // 发生错误
            setLastError("系统错误", strlen("系统错误"));
        }
        else if (reply->type == REDIS_REPLY_NIL)
        {
            // 返回值为空
            setLastError("nil", strlen("nil"));
        }
        else
        {
            // 发生错误
            setLastError(reply->str, reply->len);
        }
    }

    if (NULL != reply)
    {
        freeReplyObject(reply);
    }

    return bRet;
}

bool RedisClient::commandArgvStatus(const std::vector<std::string>& vData)
{
	if (context_ == NULL) return false;

	std::vector<const char* > argv(vData.size());
	std::vector<size_t> argvlen(vData.size());

	bool bRet = false;
	size_t i = 0;
	for (std::vector<std::string>::const_iterator it = vData.begin();
	        it != vData.end(); ++i, ++it)
	{
		argv[i] = it->c_str();
		argvlen[i] = it->size();
	}
	redisReply* reply = static_cast<redisReply* >(
	                        redisCommandArgv(context_, static_cast<int>(argv.size()), &(argv[0]), &(argvlen[0])));
	if (checkReply(reply))
	{
		bRet = true;
		if (REDIS_REPLY_STRING == reply->type)
		{
			if (!reply->len || !reply->str || strcasecmp(reply->str, "OK") != 0)
			{
				bRet = false;
			}
		}
	}
	else
	{
		if (!reply)
		{
			// 发生错误
			setLastError("系统错误", strlen("系统错误"));
		}
		else if (reply->type == REDIS_REPLY_NIL)
		{
			// 这不是一种错误
			// setLastError("nil", strlen("nil"));
		}
		else
		{
			// 发生错误
			setLastError(reply->str, reply->len);
		}
	}

	if (NULL != reply)
	{
		freeReplyObject(reply);
	}

	return bRet;
}

bool RedisClient::commandArgvInteger(const std::vector<std::string>& vData, int64_t& retval)
{
	if (context_ == NULL) return false;

	std::vector<const char* > argv(vData.size());
	std::vector<size_t> argvlen(vData.size());

	bool bRet = false;
	size_t i = 0;
	for (std::vector<std::string>::const_iterator it = vData.begin();
	        it != vData.end(); ++i, ++it)
	{
		argv[i] = it->c_str();
		argvlen[i] = it->size();
	}
	redisReply* reply = static_cast<redisReply* >(
	                        redisCommandArgv(context_, static_cast<int>(argv.size()), &(argv[0]), &(argvlen[0])));
	if (checkReply(reply))
	{
		retval = reply->integer;
		bRet = true;
	}
	else
	{
		if (!reply)
		{
			// 发生错误
			setLastError("系统错误", strlen("系统错误"));
		}
		else if (reply->type == REDIS_REPLY_NIL)
		{
			// 这不是一种错误
			// setLastError("nil", strlen("nil"));
		}
		else
		{
			// 发生错误
			setLastError(reply->str, reply->len);
		}
	}

	if (NULL != reply)
	{
		freeReplyObject(reply);
	}

	return bRet;
}

bool RedisClient::commandArgvArray(const ArrayList& vData, HashMapDataItem& dataItemMap)
{
	if (context_ == NULL) return false;

	std::vector<const char* > argv(vData.size());
	std::vector<size_t> argvlen(vData.size());

	bool bRet = false;
	size_t i = 0;
	for (std::vector<std::string>::const_iterator it = vData.begin();
	        it != vData.end(); ++i, ++it)
	{
		argv[i] = it->c_str();
		argvlen[i] = it->size();
	}
	redisReply* reply = static_cast<redisReply* >(
	                        redisCommandArgv(context_, static_cast<int>(argv.size()), &(argv[0]), &(argvlen[0])));
	if (checkReply(reply))
	{
		for (size_t j = 0; (j < reply->elements) && (j < vData.size() - 2); j++)
		{
			DataItem item;
			item.type = reply->element[j]->type;
			item.value.assign(reply->element[j]->str, reply->element[j]->len);
			dataItemMap.insert(std::make_pair(vData[j + 2], item));
			// vReply.push_back(item);
		}
		bRet = true;
	}
	else
	{
		if (!reply)
		{
			// 发生错误
			setLastError("系统错误", strlen("系统错误"));
		}
		else if (reply->type == REDIS_REPLY_NIL)
		{
			// 这不是一种错误
			// setLastError("nil", strlen("nil"));
		}
		else
		{
			// 发生错误
			setLastError(reply->str, reply->len);
		}
	}

	if (NULL != reply)
	{
		freeReplyObject(reply);
	}

	return bRet;
}

bool RedisClient::commandInteger(int64_t& retval, const char* cmd, ...)
{
	if (context_ == NULL) return false;

	bool bRet = false;
	va_list args;
	va_start(args, cmd);
	redisReply* reply = static_cast<redisReply* >(redisvCommand(context_, cmd, args));
	va_end(args);

	if (checkReply(reply))
	{
		retval = reply->integer;
		bRet = true;
	}
	else
	{
		if (!reply)
		{
			// 发生错误
			setLastError("系统错误", strlen("系统错误"));
		}
		else if (reply->type == REDIS_REPLY_NIL)
		{
			// 这不是一种错误
			// setLastError("nil", strlen("nil"));
		}
		else
		{
			// 发生错误
			setLastError(reply->str, reply->len);
		}
	}

	if (NULL != reply)
	{
		freeReplyObject(reply);
	}

	return bRet;
}

bool RedisClient::commandString(std::string& value, const char* cmd, ...)
{
	if (context_ == NULL) return false;

	bool bRet = false;
	va_list args;
	va_start(args, cmd);
	redisReply* reply = static_cast<redisReply* >(redisvCommand(context_, cmd, args));
	va_end(args);

	if (checkReply(reply))
	{
		value.assign(reply->str, reply->len);
		bRet = true;
	}
	else
	{
		if (!reply)
		{
			// 发生错误
			setLastError("系统错误", strlen("系统错误"));
		}
		else if (reply->type == REDIS_REPLY_NIL)
		{
			// 这不是一种错误
			// setLastError("nil", strlen("nil"));
		}
		else
		{
			// 发生错误
			setLastError(reply->str, reply->len);
		}
	}

	if (NULL != reply)
	{
		freeReplyObject(reply);
	}

	return bRet;
}

bool RedisClient::commandList(std::vector<std::string>& vValue, const char* cmd, ...)
{
	if (context_ == NULL) return false;

	bool bRet = false;
	va_list args;
	va_start(args, cmd);
	redisReply* reply = static_cast<redisReply* >(redisvCommand(context_, cmd, args));
	va_end(args);

	if (checkReply(reply))
	{
		for (size_t i = 0; i < reply->elements; i++)
		{
			vValue.push_back(std::string(reply->element[i]->str, reply->element[i]->len));
		}
		bRet = true;
	}
	else
	{
		if (!reply)
		{
			// 发生错误
			setLastError("系统错误", strlen("系统错误"));
		}
		else if (reply->type == REDIS_REPLY_NIL)
		{
			// 这不是一种错误
			// setLastError("nil", strlen("nil"));
		}
		else
		{
			// 发生错误
			setLastError(reply->str, reply->len);
		}
	}

	if (NULL != reply)
	{
		freeReplyObject(reply);
	}

	return bRet;
}

bool RedisClient::commandArray(HashMapDataItem& dataItemMap, const char* cmd, ...)
{
	if (context_ == NULL) return false;

	bool bRet = false;
	va_list args;
	va_start(args, cmd);
	redisReply* reply = static_cast<redisReply* >(redisvCommand(context_, cmd, args));
	va_end(args);

	if (checkReply(reply))
	{
		for (size_t i = 0; i < reply->elements; i += 2)
		{
			std::string key;
			key.assign(reply->element[i]->str, reply->element[i]->len);

			DataItem item;
			item.type = reply->element[i+1]->type;
			item.value.assign(reply->element[i+1]->str, reply->element[i+1]->len);

			dataItemMap.insert(std::make_pair(key, item));
		}
		bRet = true;
	}
	else
	{
		if (!reply)
		{
			// 发生错误
			setLastError("系统错误", strlen("系统错误"));
		}
		else if (reply->type == REDIS_REPLY_NIL)
		{
			// 这不是一种错误
			// setLastError("nil", strlen("nil"));
		}
		else
		{
			// 发生错误
			setLastError(reply->str, reply->len);
		}
	}

	if (NULL != reply)
	{
		freeReplyObject(reply);
	}

	return bRet;
}

bool RedisClient::commandBool(const char* cmd, ...)
{
	if (context_ == NULL) return false;

	bool bRet = false;
	va_list args;
	va_start(args, cmd);
	redisReply* reply = static_cast<redisReply* >(redisvCommand(context_, cmd, args));
	va_end(args);

	if (checkReply(reply))
	{
		if (REDIS_REPLY_STATUS == reply->type)
		{
			bRet = true;
		}
		else
		{
			bRet = (reply->integer == 1) ? true : false;
		}
	}
	else
	{
		if (!reply)
		{
			// 发生错误
			setLastError("系统错误", strlen("系统错误"));
		}
		else if (reply->type == REDIS_REPLY_NIL)
		{
			// 这不是一种错误
			// setLastError("nil", strlen("nil"));
		}
		else
		{
			// 发生错误
			setLastError(reply->str, reply->len);
		}
	}

	if (NULL != reply)
	{
		freeReplyObject(reply);
	}

	return bRet;
}

bool RedisClient::checkReply(const redisReply* reply)
{
	if (reply == NULL) return false;
	switch (reply->type)
	{
	case REDIS_REPLY_STRING:
	case REDIS_REPLY_ARRAY:
	case REDIS_REPLY_INTEGER:
	case REDIS_REPLY_STATUS:
		return true;

		break;
	case REDIS_REPLY_NIL:
	case REDIS_REPLY_ERROR:
	default:
		return false;
	}
}

void RedisClient::addParam(std::vector<std::string>& dest, const std::vector<std::string>& src)
{
	for (std::vector<std::string>::const_iterator it = src.begin(); it != src.end(); ++it)
	{
		dest.push_back(*it);
	}
}

void RedisClient::setLastError(const char* buffer, size_t len)
{
	if (buffer)
	{
		lastError_.assign(buffer, len);
	}
	else
	{
		lastError_.assign("系统错误");
	}

	common::Monitor::instance().writeRedisErrorMsg();
	
	// 发送异常，值连接状态为false
	connStatus_ = false;
}

bool RedisClient::exists(const std::string& key)
{
	if (key.empty() || key == "")
	{
		return false;
	}
	return commandBool("EXISTS %s", key.c_str());
}

bool RedisClient::expire(const std::string& key, unsigned int seconds)
{
	if (key.empty() || key == "")
	{
		return false;
	}

	int64_t retval = -1;
	if (!commandInteger(retval, "EXPIRE %s %u", key.c_str(), seconds))
	{
		return false;
	}

	if (1 == retval)
	{
		return true;
	}
	else
	{
		setLastError("系统错误", strlen("系统错误"));
		return false;
	}
}

bool RedisClient::set(const std::string& key, const std::string& value)
{
	if (key.empty() || key == "" || value.empty() || value == "")
	{
		return false;
	}

	std::vector<std::string> vCmdData;
	vCmdData.push_back("SET");
	vCmdData.push_back(key);
	vCmdData.push_back(value);

	return commandArgvStatus(vCmdData);
}

bool RedisClient::get(const std::string& key, std::string& value)
{
	if (key.empty() || key == "")
	{
		return false;
	}
	return commandString(value, "GET %s", key.c_str());
}

bool RedisClient::getset(const std::string& key, const std::string& newValue, std::string& oldValue)
{
	if (key.empty() || key == "" || newValue.empty() || newValue == "" )
	{
		return false;
	}
	return commandString(oldValue, "GETSET %s %s", key.c_str(), newValue.c_str());
}

bool RedisClient::del(const std::string& key)
{
	if (key.empty() || key == "")
	{
		return false;
	}
	return commandBool("DEL %s", key.c_str());
}

bool RedisClient::hset(const std::string& key, const std::string& field, const std::string& value, int64_t& retval)
{
	if (key.empty() || key == "" || field.empty() || field == "" || value.empty() || value == "")
	{
		return false;
	}

	std::vector<std::string> vCmdData;
	vCmdData.push_back("HSET");
	vCmdData.push_back(key);
	vCmdData.push_back(field);
	vCmdData.push_back(value);

	return commandArgvInteger(vCmdData, retval);
}

bool RedisClient::hget(const std::string& key, const std::string& field, std::string& value)
{
	if (key.empty() || key == "" || field.empty() || field == "" )
	{
		return false;
	}
	return commandString(value, "HGET %s %s", key.c_str(), field.c_str());
}

bool RedisClient::hgetall(const std::string& key, HashMapDataItem& dataItemMap)
{
	if (key.empty() || key == "")
	{
		return false;
	}
	return commandArray(dataItemMap, "hgetall %s", key.c_str());
}

bool RedisClient::hdel(const std::string& key, const std::string& field, int64_t& num)
{
	if (key.empty() || key == "" || field.empty() || field == "" )
	{
		return false;
	}
	return commandInteger(num, "HDEL %s %s", key.c_str(), field.c_str());
}

bool RedisClient::hmset(const std::string& key, const HashMap& hashMap, int64_t& retval)
{
	if (key.empty() || key == "" || hashMap.size() == 0)
	{
		return false;
	}

	ArrayList vCmdData;
	vCmdData.push_back("HMSET");
	vCmdData.push_back(key);
	for (HashMap::const_iterator it = hashMap.begin(); it != hashMap.end(); ++it)
	{
		vCmdData.push_back(it->first);
		vCmdData.push_back(it->second);
	}

	return commandArgvInteger(vCmdData, retval);
}

bool RedisClient::hincrby(const std::string& key, const std::string& field, const std::string& value,int64_t& retval)
{
	if (key.empty() || key == "" || field.size() == 0)
	{
		return false;
	}

	return commandInteger(retval, "HINCRBY %s %s %s", key.c_str(), field.c_str(),value.c_str());
}
bool RedisClient::incrby(const std::string& key, const int value, int64_t& retval)
{
    if (key.empty() || key == "")
    {
        return false;
    }

    return commandInteger(retval, "INCRBY %s %d", key.c_str(), value);
}


bool RedisClient::hmget(const std::string& key, const ArrayList& fieldList, HashMapDataItem& dataItemMap)
{
	if (key.empty() || key == "" || fieldList.size() == 0)
	{
		return false;
	}
	std::vector<std::string> vCmdData;
	vCmdData.push_back("HMGET");
	vCmdData.push_back(key);
	addParam(vCmdData, fieldList);

	return commandArgvArray(vCmdData, dataItemMap);
}

bool RedisClient::sadd(const std::string& key, const std::vector<std::string>& vValue, int64_t& retval)
{
	if (key.empty() || key == "" || vValue.size() == 0)
	{
		return false;
	}

	std::vector<std::string> vCmdData;
	vCmdData.push_back("SADD");
	vCmdData.push_back(key);
	addParam(vCmdData, vValue);

	return commandArgvInteger(vCmdData, retval);
}

bool RedisClient::scard(const std::string& key, int64_t& count)
{
	if (key.empty() || key == "")
	{
		return false;
	}
	return commandInteger(count, "SCARD %s", key.c_str());
}

bool RedisClient::smembers(const std::string& key, std::vector<std::string>& vValue)
{
	if (key.empty() || key == "")
	{
		return false;
	}

	return commandList(vValue, "SMEMBERS %s", key.c_str());
}

bool RedisClient::srem(const std::string& key, const std::vector<std::string>& members, int64_t& count)
{
	if (key.empty() || key == "" || members.size() == 0)
	{
		return false;
	}

	std::vector<std::string> vCmdData;
	vCmdData.push_back("SREM");
	vCmdData.push_back(key);
	addParam(vCmdData, members);

	return commandArgvInteger(vCmdData, count);
}

bool RedisClient::sismember(const std::string& key, const std::string& member)
{
	if (key.empty() || key == "")
	{
		return false;
	}

	return commandBool("SISMEMBER %s %s", key.c_str(), member.c_str());
}

bool RedisClient::zadd(const std::string& key, int score, const std::string& member, int64_t& retval)
{
	if (key.empty() || key == "" || member.empty() || member == "")
	{
		return false;
	}

	return commandInteger(retval, "ZADD %s %d %s", key.c_str(), score, member.c_str());
}

bool RedisClient::zadd(const std::string& key, double score, const std::string& member, int64_t& retval)
{
	if (key.empty() || key == "" || member.empty() || member == "")
	{
		return false;
	}

	return commandInteger(retval, "ZADD %s %f %s", key.c_str(), score, member.c_str());
}

bool RedisClient::zadd(const std::string& key, const std::string& score, const std::string& member, int64_t& retval)
{
	if (key.empty() || key == "" || score.empty() || score == "" || member.empty() || member == "")
	{
		return false;
	}

	return commandInteger(retval, "ZADD %s %s %s", key.c_str(), score.c_str(), member.c_str());
}

bool RedisClient::zadd(const std::string& key, const std::map<std::string, std::string>& mMap, int64_t& retval)
{
	if (key.empty() || key == "" || mMap.size() == 0)
	{
		return false;
	}

	std::vector<std::string> vCmdData;
	vCmdData.push_back("ZADD");
	vCmdData.push_back(key);

	for (std::map<std::string, std::string>::const_iterator it = mMap.begin(); it != mMap.end(); ++it)
	{
		vCmdData.push_back(it->second);
		vCmdData.push_back(it->first);
	}

	return commandArgvInteger(vCmdData, retval);
}

bool RedisClient::zcard(const std::string& key, int64_t& count)
{
	if (key.empty() || key == "")
	{
		return false;
	}
	return commandInteger(count, "ZCARD %s", key.c_str());
}

bool RedisClient::zincrby(const std::string& key, const double& increment,
                          const std::string& member, std::string& scoreResult)
{
	if (key.empty() || key == "")
	{
		return false;
	}

	return commandString(scoreResult, "ZINCRBY %s %f %s", key.c_str(), increment, member.c_str());
}

bool RedisClient::zcount(const std::string& key, double minScore, double maxScore, int64_t& count)
{
	if (key.empty() || key == "")
	{
		return false;
	}

	return commandInteger(count, "ZCOUNT %s %f %f", key.c_str(), minScore, maxScore);
}

bool RedisClient::zrange(const std::string& key, int start, int end, std::vector<std::string>& vValue, bool withScore)
{
	if (key.empty() || key == "")
	{
		return false;
	}

	if (withScore)
	{
		return commandList(vValue, "ZRANGE %s %d %d WITHSCORES", key.c_str(), start, end);
	}
	return commandList(vValue, "ZRANGE %s %d %d", key.c_str(), start, end);
}

bool RedisClient::zrem(const std::string& key, const std::vector<std::string>& vMembers, int64_t retval)
{
	if (key.empty() || key == "" || vMembers.size() == 0)
	{
		return false;
	}

	std::vector<std::string> vCmdData;
	vCmdData.push_back("ZREM");
	vCmdData.push_back(key);
	addParam(vCmdData, vMembers);

	return commandArgvInteger(vCmdData, retval);
}

bool RedisClient::zrangebylex(const std::string& key,
                              const std::string& start,
                              const std::string& end,
                              std::vector<std::string>& vValue,
                              int offset, int count)
{
	if (key.empty() || key == "" || start.empty() || start == "" || end.empty() || end == "")
	{
		return false;
	}

	return commandList(vValue, "ZRANGEBYLEX %s %s %s LIMIT %d %d",
	                   key.c_str(), start.c_str(), end.c_str(), offset, count);
}

bool RedisClient::zrangebyscore(const std::string& key,
                                const std::string& start,
                                const std::string& end,
                                std::vector<std::string>& vValue,
                                int offset, int count)
{
	if (key.empty() || key == "" || start.empty() || start == "" || end.empty() || end == "")
	{
		return false;
	}

	return commandList(vValue, "ZRANGEBYSCORE %s %s %s LIMIT %d %d",
	                   key.c_str(), start.c_str(), end.c_str(), offset, count);
}

bool RedisClient::zrevrangebyscore(const std::string& key,
						std::vector<std::string>& vValue,
						const std::string& start,
						const std::string& end,
						int offset,int cout)
{
	if(key.empty() || key == "")
	{
			return false;
	}

	if(start.empty() && end.empty())
	{
		if(offset == cout && cout == 0)
		{
			return commandList(vValue, "ZREVRANGEBYSCORE %s -inf +inf",key.c_str());
		}else{
			return commandList(vValue, "ZREVRANGEBYSCORE %s -inf +inf LIMIT %d %d",key.c_str(), offset, cout);
		}
	}else{
		if(offset == cout && cout == 0)
		{
			return commandList(vValue, "ZREVRANGEBYSCORE %s %s %s",
	                   key.c_str(), start.c_str(), end.c_str());
		}else{
			return commandList(vValue, "ZREVRANGEBYSCORE %s %s %s LIMIT %d %d",
	                   key.c_str(), start.c_str(), end.c_str(), offset, cout);
		}
	}
}

bool RedisClient::zrangebyscore_(const std::string& key,
						std::vector<std::string>& vValue,
						const std::string& start,
						const std::string& end,
						int offset,int cout)
{
	if(key.empty() || key == "")
	{
			return false;
	}

	if(start.empty() && end.empty())
	{
		if(offset == cout && cout == 0)
		{
			return commandList(vValue, "ZRANGEBYSCORE %s -inf +inf WITHSCORES",key.c_str());
		}else{
			return commandList(vValue, "ZRANGEBYSCORE %s -inf +inf WITHSCORES LIMIT %d %d",key.c_str(), offset, cout);
		}
	}else{
		if(offset == cout && cout == 0)
		{
			return commandList(vValue, "ZRANGEBYSCORE %s %s %s WITHSCORES",
	                   key.c_str(), start.c_str(), end.c_str());
		}else{
			return commandList(vValue, "ZRANGEBYSCORE %s %s %s WITHSCORES LIMIT %d %d",
	                   key.c_str(), start.c_str(), end.c_str(), offset, cout);
		}
	}
}

bool RedisClient::zpopminbyscript(std::vector<std::string> &vValue, const char *cmd)
{
    if (NULL == cmd)
	{
		return false;
	}

	return commandList(vValue, cmd);
}

bool RedisClient::lpush(const std::string& key, const std::string& value,int64_t& retval)
{
    if (key.empty() || key == "" || value.empty() || value == "")
    {
        return false;
    }

    std::vector<std::string> vCmdData;
    vCmdData.push_back("LPUSH");
    vCmdData.push_back(key);
    vCmdData.push_back(value);

    return commandArgvInteger(vCmdData, retval);
}

bool RedisClient::rpush(const std::string& key, const std::string& value,int64_t& retval)
{
    if (key.empty() || key == "" || value.empty() || value == "")
    {
        return false;
    }

    std::vector<std::string> vCmdData;
    vCmdData.push_back("RPUSH");
    vCmdData.push_back(key);
    vCmdData.push_back(value);

    return commandArgvInteger(vCmdData, retval);
}

bool RedisClient::lpop(const std::string& key, std::string& value)
{
    if (key.empty() || key == "")
    {
        return false;
    }
    std::string cmd = "LPOP " + key;
    return commandString(value, cmd.c_str());
}

bool RedisClient::rpop(const std::string& key, std::string& value)
{
    if (key.empty() || key == "")
    {
        return false;
    }
    std::string cmd = "RPOP " + key;
    return commandString(value, cmd.c_str());
}

bool RedisClient::blpop(const std::string& key, std::vector<std::string>& vValue, int64_t timeout)
{
    if (key.empty() || key == "")
    {
        return false;
    }
    std::string cmd = "BLPOP " + key + " " + std::to_string(timeout);
    return commandList(vValue, cmd.c_str());
    
}

bool RedisClient::brpop(const std::string& key, std::vector<std::string>& vValue, int64_t timeout)
{
    if (key.empty() || key == "")
    {
        return false;
    }
    std::string cmd = "BRPOP " + key + " " + std::to_string(timeout);
    return commandList(vValue, cmd.c_str());
}
