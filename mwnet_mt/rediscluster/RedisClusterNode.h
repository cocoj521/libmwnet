#ifndef __REDIS_CLUSTER_NODE_H__
#define __REDIS_CLUSTER_NODE_H__

//#include <common/MutexLock.h>
#include <vector>
#include <list>
#include <map>
#include <mutex>

extern const int kMinSize;
extern const int kMaxSize;

namespace rediscluster
{

typedef struct __SlotsInfo
{
	uint16_t slotStart;
	uint16_t slotEnd;
} SlotsInfo;

typedef struct __NodeInfo
{
	std::string id;
	std::string ipStr;
	int port;
	std::vector<SlotsInfo> slots;

	const std::vector<SlotsInfo>& getSlotsVec() const
	{
		return slots;
	} 

	bool ParseNodeString(const std::string& nodeStr)
	{
		std::string::size_type pos = nodeStr.find(':');
		if (pos == std::string::npos)
		{
			return false;
		}
		else
		{
			const std::string portStr = nodeStr.substr(pos + 1);
			port  = atoi(portStr.c_str());
			ipStr = nodeStr.substr(0, pos);
			return true;
		}
	}

	void ParseSlotString(const std::string& slotStr)
	{
		SlotsInfo slotsInfo; 
		std::string::size_type pos = slotStr.find('-');
		if (pos == std::string::npos)
		{
			slotsInfo.slotStart = static_cast<uint16_t>(atoi(slotStr.c_str()));
			slotsInfo.slotEnd = slotsInfo.slotStart;
		}
		else
		{
			const std::string slotEndStr = slotStr.substr(pos + 1);
			slotsInfo.slotEnd  = static_cast<uint16_t>(atoi(slotEndStr.c_str()));
			slotsInfo.slotStart = static_cast<uint16_t>(atoi(slotStr.substr(0, pos).c_str()));
		}
		slots.push_back(slotsInfo);
	}

} NodeInfo;

class RedisClient;
class RedisClusterNode
{
public:
	RedisClusterNode();
	~RedisClusterNode();

	bool init(const NodeInfo& info, int minSize, int maxSize, int timeout, const std::string& password = "");
	RedisClient* getRedisClient();
	bool getStatus() const { return bStatus_; }
	void setStatus(bool bStatus) { bStatus_ = bStatus; }
	void releaseRedisClient(RedisClient* client);
	void removeRedisClient(RedisClient* client);
	void stop();
	const std::string& getNodeId() const { return info_.id; }

private:
	RedisClusterNode(const RedisClusterNode& ) = delete;
	RedisClusterNode& operator=(const RedisClusterNode& ) = delete;

	bool applayNewRedisClient(RedisClient*& client);

private:
	bool bStatus_;
	NodeInfo info_;
	int minSize_;
	int maxSize_;
	int currSize_;
	int timeout_;
	std::string password_;

	std::map<RedisClient*, int> useMap_;
	std::list<RedisClient* > idleList_;
	//common::MutexLock mutex_;
	std::mutex m_lock;
};

} // end namespace rediscluster 

#endif // __REDIS_CLUSTER_NODE_H__
