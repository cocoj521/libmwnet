#include "RedisClusterNode.h"
#include "RedisClient.h"


using namespace rediscluster;

const int kMinSize = 1;
const int kMaxSize = 64;

RedisClusterNode::RedisClusterNode()
	: bStatus_(true),
	  minSize_(2),
	  maxSize_(10),
	  currSize_(0),
	  timeout_(0),
	  password_("")
{
}

RedisClusterNode::~RedisClusterNode()
{
	printf("host=%s port=%d will disconnect", info_.ipStr.c_str(), info_.port);
	stop();
}

bool RedisClusterNode::init(const NodeInfo& info,
                            int minSize, int maxSize, int timeout,
                            const std::string& password)
{
	info_ = info;
	minSize_ = minSize < kMinSize ? kMinSize : minSize;
	maxSize_ = maxSize > kMaxSize ? kMaxSize : maxSize;
	timeout_ = timeout;
	password_ = password;

	if (minSize_ > maxSize_)
	{
		return false;
	}

	for (int i = 0; i < minSize_; ++i)
	{
		RedisClient* client = NULL;
		if (!applayNewRedisClient(client))
		{
			return false;
		}

		idleList_.push_back(client);
	}
	return true;
}


void RedisClusterNode::stop()
{
	//common::MutexLockGuard guard(mutex_);
	std::lock_guard<std::mutex> lock_(m_lock);
	std::list<RedisClient* >::iterator it = idleList_.begin();
	for ( ; it != idleList_.end(); ++it)
	{
		if (*it)
		{
			(*it)->release();
			delete (*it);
			*it = NULL;
		}

	}
	idleList_.clear();

	std::map<RedisClient* , int>::iterator it2 = useMap_.begin();
	for ( ; it2 != useMap_.end(); ++it2)
	{
		if (it2->first)
		{
			it2->first->release();
			delete it2->first;
		}
	}
	useMap_.clear();
}

bool RedisClusterNode::applayNewRedisClient(RedisClient*& client)
{
	if (client == NULL)
	{
		client = new RedisClient(this);
		if (client)
		{
			client->init(info_.ipStr, info_.port, timeout_, password_);
			bool bRet = client->connect();
			if (!bRet && client)
			{
				printf("conn redis failed! reason: (%s:%d) %s", info_.ipStr.c_str(), info_.port, client->getLastError().c_str());
				delete client;
				client = NULL;
			}
			return bRet;
		}
	}
	return false;
}

RedisClient* RedisClusterNode::getRedisClient()
{
	RedisClient* client = NULL;
	{
		//common::MutexLockGuard guard(mutex_);
		std::lock_guard<std::mutex> lock_(m_lock);
		if (!idleList_.empty())
		{
			client = idleList_.front();
			idleList_.pop_front();
			useMap_.insert(std::make_pair(client, 0));
		}
		else if (idleList_.empty() && static_cast<int>(useMap_.size()) < maxSize_)
		{
			if (applayNewRedisClient(client))
			{
				useMap_.insert(std::make_pair(client, 0));
			}
			else if(useMap_.empty())
			{
				// 如果申请新的连接不成功+无再用连接,则置节点状态为false
				bStatus_ = false;
			}
		}
	}
	return client;
}

void RedisClusterNode::releaseRedisClient(RedisClient* client)
{
	if (client != NULL)
	{
		//common::MutexLockGuard guard(mutex_);
		std::lock_guard<std::mutex> lock_(m_lock);
		if (useMap_.find(client) != useMap_.end())
		{
			idleList_.push_back(client);
			useMap_.erase(client);
			//DEBUG("releaseRedisClient success, idleList_.size(): %lu", idleList_.size());
		}
		else
		{
			//DEBUG("releaseRedisClient failed, idleList_.size(): %lu", idleList_.size());
		}
	}
}

void RedisClusterNode::removeRedisClient(RedisClient* client)
{
	if (client != NULL)
	{
		//common::MutexLockGuard guard(mutex_);
		std::lock_guard<std::mutex> lock_(m_lock);
		if (useMap_.find(client) != useMap_.end())
		{
			useMap_.erase(client);
			client->release();
			client = NULL;

			//如果空闲 + 在用 size = 0
			if (useMap_.empty() && idleList_.empty())
			{
				bStatus_ = false;
			}
		}
		
	}
}
