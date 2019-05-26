#ifndef __REDIS_CLUSTER_MGR_H__
#define __REDIS_CLUSTER_MGR_H__

#include "RedisClusterNode.h"
#include <hiredis/hiredis.h>

#include <common/Singleton.h>
//#include <common/MutexLock.h>
#include <string>
#include <vector>
#include <map>
#include <memory>

#define CONNECT_SUCCESS 0 

namespace rediscluster
{

typedef std::shared_ptr<RedisClusterNode> RedisClusterNodePtr;

class RedisClusterMgr
{
public:
	RedisClusterMgr()
		: bStatus_(true),
		  ipStr_("127.0.0.1"),
		  port_(7000),
		  minSize_(1),
		  maxSize_(8),
		  timeout_(15),
		  password_("")
	{
	}

	~RedisClusterMgr() = default;

	int  init(const std::string& ipStr,
	          int port,
	          int minSize,
	          int maxSize,
	          int timeout = 0,
	          const std::string& passwd = "",
	          int fullCoverage = 0/*是否要求slot全覆盖*/);
	int  init2(const std::string& brokers,
	           int minSize,
	           int maxSize,
	           int timeout = 0,
	           const std::string& passwd = "",
	           int fullCoverage = 0/*是否要求slot全覆盖*/);
	int  start();
	void stop();

	static RedisClusterMgr& instance() { return common::Singleton<RedisClusterMgr>::instance(); }

	RedisClient* findNodeConnection(const std::string& key);

private:
	uint16_t crc16(const char *buf, size_t len);

	bool clusterEnabled(redisContext* context);

	bool clusterInfo(redisContext* context);

	bool getClusterNodesInfo(redisContext* context);

	bool str2Vec(const char* src, std::vector<std::string>& dest, const char* delim);

	bool connectClusterNode(const NodeInfo& node);

	RedisClient* getConnection(uint16_t slotIndex);

	uint16_t getKeySlotIndex(const char* key);

	uint16_t keyHashSlot(const char* key, size_t len);

	// const std::string& findNodeId(uint16_t slotIndex);

	void split(const std::string& str, const std::string& delim, std::vector<std::string>* result);

private:
	RedisClusterMgr(RedisClusterMgr& ) = delete;
	RedisClusterMgr& operator=(RedisClusterMgr& ) = delete;

private:
	bool bStatus_;
	int fullCoverage_;
	std::string brokers_;
	std::string ipStr_;
	int port_;
	int minSize_;
	int maxSize_;
	int timeout_;
	std::string password_;
	std::vector<NodeInfo> nodes_;
	std::map<std::string, bool> nodesStatus_;
	std::map<uint16_t, RedisClusterNodePtr> slotMap_;

	//common::MutexLock initMtx_;
	std::mutex m_lock;
};

} // end namespace rediscluster

#endif // __REDIS_CLUSTER_MGR_H__
