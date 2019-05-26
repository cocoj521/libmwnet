#ifndef __REDIS_CLIENT_H__
#define __REDIS_CLIENT_H__

#include "RedisClusterNode.h"

#include <stdint.h>
#include <string>
#include <vector>
#include <map>
#include <string.h>

#if defined(__clang__) || __GNUC_PREREQ (4,6)
#pragma GCC diagnostic push
#endif
#pragma GCC diagnostic ignored "-Wold-style-cast"

#include <hiredis/hiredis.h>

#if defined(__clang__) || __GNUC_PREREQ (4,6)
#pragma GCC diagnostic pop
#else
#pragma GCC diagnostic warning "-Wold-style-cast"
#endif



#define GET_CONNECT_ERROR    "get connection error"
#define CONNECT_CLOSED_ERROR "redis connection be closed"

typedef struct _DATA_ITEM_
{
	/**
	 * 参见hiredis库头文件：read.h
	 * REDIS_REPLY_STRING 1
	 * REDIS_REPLY_ARRAY 2
	 * REDIS_REPLY_INTEGER 3
	 * REDIS_REPLY_NIL 4
	 * REDIS_REPLY_STATUS 5
	 * REDIS_REPLY_ERROR 6
	 */
	int type;
	std::string value;

	_DATA_ITEM_& operator=(const _DATA_ITEM_& data)
	{
		type = data.type;
		value = data.value;
		return *this;
	}
    int intValue(){ return atoi(value.c_str()); }
} DataItem;

typedef std::map<std::string, DataItem> HashMapDataItem;
typedef std::vector<std::string> ArrayList;
typedef std::map<std::string, std::string> HashMap;

namespace rediscluster
{

class RedisClient
{
public:
	explicit RedisClient(RedisClusterNode* manager);
	~RedisClient();

	bool init(const std::string& ipStr, int port, int timeout, const std::string& password = "");
	bool connect();
	bool reConnect();
	bool auth();
	bool ping();
	void release();
	void recycle();

	redisContext* getContext() const { return context_; }
	const std::string& getLastError() const { return lastError_; }

	//执行LUA脚本命令
	bool command(std::string& result, const char* cmd, ...);
	bool command(HashMapDataItem& result, const char* cmd, ...);
    bool commandLuaList(std::vector<std::string>& vValue, const char* cmd, ...);


	// 通用操作
	bool exists(const std::string& key);

	bool expire(const std::string& key, unsigned int seconds);

	// string操作
	bool set(const std::string& key, const std::string& value);
	bool get(const std::string& key, std::string& value);
	bool getset(const std::string& key, const std::string& newValue, std::string& oldValue);
	bool del(const std::string& key);
	// bool del(const std::vector<std::string>& keys);

	// hash操作
	bool hset(const std::string& key, const std::string& field, const std::string& value, int64_t& retval);
	bool hget(const std::string& key, const std::string& field, std::string& value);
	bool hdel(const std::string& key, const std::string& field, int64_t& retval);
	bool hgetall(const std::string& key, HashMapDataItem& dataItemMap);
	// bool hdel(const std::string& key, const std::vector<std::string>& vfiled, int64_t& num);

	bool hmset(const std::string& key, const HashMap& hashMap, int64_t& retval);
	bool hmget(const std::string& key, const ArrayList& fieldList, HashMapDataItem& dataItemMap);
	
	//
	bool hincrby(const std::string& key, const std::string& field, const std::string& value,int64_t& retval);
    bool incrby(const std::string& key, const int value, int64_t& retval);
    inline bool incr(const std::string& key, int64_t& retval)
    {
        return incrby(key, 1, retval);
    }

	// 无序set操作
	bool sadd(const std::string& key, const std::vector<std::string>& vValue, int64_t& retval);
	bool scard(const std::string& key, int64_t& count);
	bool smembers(const std::string& key, std::vector<std::string>& vValue);
	bool srem(const std::string& key, const std::vector<std::string>& members, int64_t& count);
	bool sismember(const std::string& key, const std::string& member);

    //队列
    bool lpush(const std::string& key, const std::string& value,int64_t& retval);
    bool rpush(const std::string& key, const std::string& value,int64_t& retval);
    bool lpop(const std::string& key, std::string& value);
    bool rpop(const std::string& key, std::string& value);
    bool blpop(const std::string& key, std::vector<std::string>& vValue, int64_t timeout=0);
    bool brpop(const std::string& key, std::vector<std::string>& vValue, int64_t timeout=0);

	// 有序set操作
	/**
	 * [zadd 向有序SET添加一个或多个成员，或者更新已存在成员的分数]
	 * @author dengyu 2018-01-17
	 * @param  key    [主键]
	 * @param  score  [分数值]
	 * @param  member [成员]
	 * @param  retval [返回操作成功的个数]
	 * @return        [true: 成功 false: 失败]
	 */
	bool zadd(const std::string& key, int score, const std::string& member, int64_t& retval);

	/**
	 * [zadd 向有序SET添加一个或多个成员，或者更新已存在成员的分数]
	 * @author dengyu 2018-01-17
	 * @param  key    [主键]
	 * @param  score  [分数值]
	 * @param  member [成员]
	 * @param  retval [返回操作成功的个数]
	 * @return        [true: 成功 false: 失败]
	 */
	bool zadd(const std::string& key, double score, const std::string& member, int64_t& retval);

	/**
	 * [zadd 向有序SET添加一个或多个成员，或者更新已存在成员的分数]
	 * @author dengyu 2018-01-17
	 * @param  key    [主键]
	 * @param  score  [分数值]
	 * @param  member [成员]
	 * @param  retval [返回操作成功的个数]
	 * @return        [true: 成功 false: 失败]
	 */
	bool zadd(const std::string& key, const std::string& score, const std::string& member, int64_t& retval);

	/**
	 * [zadd 向有序SET添加一个或多个成员，或者更新已存在成员的分数]
	 * @author dengyu 2018-01-17
	 * @param  key    [主键]
	 * @param  mMap   [成员和分数的映射表]
	 * @param  retval [返回操作成功的个数]
	 * @return        [true: 成功 false: 失败]
	 */
	bool zadd(const std::string& key, const std::map<std::string, std::string>& mMap, int64_t& retval);

	bool zcard(const std::string& key, int64_t& count);

	/**
	 * [增加SET中对应成员的score值，并返回增加后的score值]
	 * @author dengyu 2018-01-17
	 * @param  key         [主键]
	 * @param  increment   [增加的分数值]
	 * @param  member      [成员]
	 * @param  scoreResult [增加后的score值]
	 * @return             [true: 成功 false: 失败]
	 */
	bool zincrby(const std::string& key, const double& increment, const std::string& member, std::string& scoreResult);

	/**
	 * [zcount 计算有序SET中成员分数区间中的个数]
	 * @author dengyu 2018-01-17
	 * @param  key      [主键]
	 * @param  minScore [最小的分数值]
	 * @param  maxScore [最大的分数值]
	 * @param  count    [返回的成员个数]
	 * @return          [true: 成功 false: 失败]
	 */
	bool zcount(const std::string& key, double minScore, double maxScore, int64_t& count);

	/**
	 * [zrange 按索引区间返回SET所有成员(和score的值)]
	 * @author dengyu 2018-01-17
	 * @param  key       [主键]
	 * @param  start     [开始的索引从0开始计算]
	 * @param  end       [结束的索引]
	 * @param  vValue    [返回的结果值]
	 * @param  withScore [是否把成员的score也返回]
	 * @return           [true: 成功 false: 失败]
	 */
	bool zrange(const std::string& key, int start, int end, std::vector<std::string>& vValue, bool withScore);

	/**
	 * [zrem 删除有序SET中的一个或多个成员]
	 * @author dengyu 2018-01-17
	 * @param  key      [主键]
	 * @param  vMembers [要删除的成员集合]
	 * @param  retval   [删除成功的个数]
	 * @return          [true: 成功 false: 失败]
	 */
	bool zrem(const std::string& key, const std::vector<std::string>& vMembers, int64_t retval);

	/**
	 * [zrangebylex 通过字典区间返回有序SET的成员]
	 * @author dengyu 2018-01-17
	 * @param  key    [主键]
	 * @param  start  [字典开始域，如：-, [a, (a 等]
	 * @param  end    [字典结束域，如：+, z], z) 等]
	 * @param  vValue [返回所有服务条件的成员]
	 * @param  offset [页码，从0开始计]
	 * @param  count  [每页显示的数量]
	 * @return        [true: 成功 false: 失败]
	 */
	bool zrangebylex(const std::string& key,
	                 const std::string& start,
	                 const std::string& end,
	                 std::vector<std::string>& vValue,
	                 int offset = 0, int count = 1024);
	/* 返回有序集合中指定分数区间的成员列表,有序集成员按分数值递增(从小到大)次序排列
	 *具有相同分数值的成员按字典序来排列(该属性是有序集提供的，不需要额外的计算),返回值无分数
	 *
	 *@param  key    [主键]
	 *@param  start  [分数开始区间，(5 -inf ]
	 *@param  end    [分数结束区间，(10 +inf ]
	 *@param  vValue [返回所有满足条件的成员]
	 *@param  offset [页码，从0开始计]
	 *@param  count  [每页显示的数量，如果与offset同时为0，则返回全部]
	 *@return        [true: 成功 false: 失败]
	 */
	bool zrangebyscore(const std::string& key,
	                   const std::string& start,
	                   const std::string& end,
	                   std::vector<std::string>& vValue,
	                   int offset = 0, int count = 1024);

	/*返回有序集中指定分数区间内的所有的成员,有序集成员按分数值递减(从大到小)的次序排列
	 *具有相同分数值的成员按字典序的逆序排列，返回值中无分数
	 *
	 *@param  key    [主键]
	 *@param  start  [分数开始区间，-inf 可以为空，如果为空必须与end通知保持为空，返回全部程序]
	 *@param  end    [分数结束区间，+inf 可以为空，如果为空必须与end通知保持为空，返回全部程序]
	 *@param  vValue [返回所有满足条件的成员]
	 *@param  offset [页码，从0开始计]
	 *@param  count  [每页显示的数量，如果与offset同时为0，则返回全部]
	 *@return        [true: 成功 false: 失败]
	 */
	bool zrevrangebyscore(const std::string& key,
						std::vector<std::string>& vValue,
						const std::string& start = "",
						const std::string& end = "",
						int offset = 0,int cout = 0);

	/*返回有序集合中指定分数区间的成员列表,有序集成员按分数值递增(从小到大)次序排列
	 *具有相同分数值的成员按字典序来排列(该属性是有序集提供的，不需要额外的计算),返回值有分数
	 *
	 *@param  key    [主键]
	 *@param  start  [分数开始区间，(5 -inf ]
	 *@param  end    [分数结束区间，(10 +inf ]
	 *@param  vValue [返回所有满足条件的成员]
	 *@param  offset [页码，从0开始计]
	 *@param  count  [每页显示的数量，如果与offset同时为0，则返回全部]
	 *@return        [true: 成功 false: 失败]
	 */
	bool zrangebyscore_(const std::string& key,
						std::vector<std::string>& vValue,
						const std::string& start = "",
						const std::string& end = "",
						int offset = 0,int cout = 0);
    /* input a cmd to get a record */
    bool zpopminbyscript(std::vector<std::string> &vValue, const char *cmd);
	bool getStatus(){ return connStatus_; }
private:
	redisContext* connectWithTimeout();

	bool commandArgvStatus(const std::vector<std::string>& vData);
	bool commandArgvInteger(const std::vector<std::string>& vData, int64_t& retval);
	bool commandArgvArray(const ArrayList& vData, HashMapDataItem& dataItemMap);

	bool commandInteger(int64_t& retval, const char* cmd, ...);
	bool commandString(std::string& value, const char* cmd, ...);
	bool commandList(std::vector<std::string>& vValue, const char* cmd, ...);
	bool commandArray(HashMapDataItem& dataItemMap, const char* cmd, ...);
	bool commandBool(const char* cmd, ...);

	bool checkReply(const redisReply* reply);
	void setLastError(const char* buffer, size_t len);
	void addParam(std::vector<std::string>& dest, const std::vector<std::string>& src);

private:
	bool connStatus_;
	int port_;
	int timeout_;
	std::string ipStr_;
	std::string password_;
	std::string lastError_;
	redisContext* context_;
	RedisClusterNode* manager_;
};

class RedisClientPtr
{
public:
	RedisClientPtr(RedisClient* client)
		: client_(client)
	{
	}

	~RedisClientPtr()
	{
		if (client_)
		{
			client_->recycle();
		}
	}

private:
	RedisClientPtr(RedisClientPtr& ) = delete;
	RedisClientPtr& operator=(RedisClientPtr& ) = delete;

private:
	RedisClient* client_;
};

} // end namespace rediscluster

#endif // __REDIS_CLIENT_H__
