#include "RedisClusterMgr.h"
#include "RedisClient.h"
// #include <iostream>

using namespace rediscluster;

static const std::string nullStr = "";

static const uint16_t crc16tab[256] =
{
	0x0000, 0x1021, 0x2042, 0x3063, 0x4084, 0x50a5, 0x60c6, 0x70e7,
	0x8108, 0x9129, 0xa14a, 0xb16b, 0xc18c, 0xd1ad, 0xe1ce, 0xf1ef,
	0x1231, 0x0210, 0x3273, 0x2252, 0x52b5, 0x4294, 0x72f7, 0x62d6,
	0x9339, 0x8318, 0xb37b, 0xa35a, 0xd3bd, 0xc39c, 0xf3ff, 0xe3de,
	0x2462, 0x3443, 0x0420, 0x1401, 0x64e6, 0x74c7, 0x44a4, 0x5485,
	0xa56a, 0xb54b, 0x8528, 0x9509, 0xe5ee, 0xf5cf, 0xc5ac, 0xd58d,
	0x3653, 0x2672, 0x1611, 0x0630, 0x76d7, 0x66f6, 0x5695, 0x46b4,
	0xb75b, 0xa77a, 0x9719, 0x8738, 0xf7df, 0xe7fe, 0xd79d, 0xc7bc,
	0x48c4, 0x58e5, 0x6886, 0x78a7, 0x0840, 0x1861, 0x2802, 0x3823,
	0xc9cc, 0xd9ed, 0xe98e, 0xf9af, 0x8948, 0x9969, 0xa90a, 0xb92b,
	0x5af5, 0x4ad4, 0x7ab7, 0x6a96, 0x1a71, 0x0a50, 0x3a33, 0x2a12,
	0xdbfd, 0xcbdc, 0xfbbf, 0xeb9e, 0x9b79, 0x8b58, 0xbb3b, 0xab1a,
	0x6ca6, 0x7c87, 0x4ce4, 0x5cc5, 0x2c22, 0x3c03, 0x0c60, 0x1c41,
	0xedae, 0xfd8f, 0xcdec, 0xddcd, 0xad2a, 0xbd0b, 0x8d68, 0x9d49,
	0x7e97, 0x6eb6, 0x5ed5, 0x4ef4, 0x3e13, 0x2e32, 0x1e51, 0x0e70,
	0xff9f, 0xefbe, 0xdfdd, 0xcffc, 0xbf1b, 0xaf3a, 0x9f59, 0x8f78,
	0x9188, 0x81a9, 0xb1ca, 0xa1eb, 0xd10c, 0xc12d, 0xf14e, 0xe16f,
	0x1080, 0x00a1, 0x30c2, 0x20e3, 0x5004, 0x4025, 0x7046, 0x6067,
	0x83b9, 0x9398, 0xa3fb, 0xb3da, 0xc33d, 0xd31c, 0xe37f, 0xf35e,
	0x02b1, 0x1290, 0x22f3, 0x32d2, 0x4235, 0x5214, 0x6277, 0x7256,
	0xb5ea, 0xa5cb, 0x95a8, 0x8589, 0xf56e, 0xe54f, 0xd52c, 0xc50d,
	0x34e2, 0x24c3, 0x14a0, 0x0481, 0x7466, 0x6447, 0x5424, 0x4405,
	0xa7db, 0xb7fa, 0x8799, 0x97b8, 0xe75f, 0xf77e, 0xc71d, 0xd73c,
	0x26d3, 0x36f2, 0x0691, 0x16b0, 0x6657, 0x7676, 0x4615, 0x5634,
	0xd94c, 0xc96d, 0xf90e, 0xe92f, 0x99c8, 0x89e9, 0xb98a, 0xa9ab,
	0x5844, 0x4865, 0x7806, 0x6827, 0x18c0, 0x08e1, 0x3882, 0x28a3,
	0xcb7d, 0xdb5c, 0xeb3f, 0xfb1e, 0x8bf9, 0x9bd8, 0xabbb, 0xbb9a,
	0x4a75, 0x5a54, 0x6a37, 0x7a16, 0x0af1, 0x1ad0, 0x2ab3, 0x3a92,
	0xfd2e, 0xed0f, 0xdd6c, 0xcd4d, 0xbdaa, 0xad8b, 0x9de8, 0x8dc9,
	0x7c26, 0x6c07, 0x5c64, 0x4c45, 0x3ca2, 0x2c83, 0x1ce0, 0x0cc1,
	0xef1f, 0xff3e, 0xcf5d, 0xdf7c, 0xaf9b, 0xbfba, 0x8fd9, 0x9ff8,
	0x6e17, 0x7e36, 0x4e55, 0x5e74, 0x2e93, 0x3eb2, 0x0ed1, 0x1ef0
};

#if defined(__clang__) || __GNUC_PREREQ (4,6)
#pragma GCC diagnostic push
#endif
#pragma GCC diagnostic ignored "-Wconversion"

uint16_t RedisClusterMgr::crc16(const char *buf, size_t len)
{
	size_t counter;
	uint16_t crc = 0;
	for (counter = 0; counter < len; counter++)
		crc = (crc << 8) ^ crc16tab[((crc >> 8) ^ *buf++) & 0x00FF];
	return crc;
}

#if defined(__clang__) || __GNUC_PREREQ (4,6)
#pragma GCC diagnostic pop
#else
#pragma GCC diagnostic warning "-Wconversion"
#endif

int RedisClusterMgr::init(const std::string& ipStr,
                          int port, int minSize, int maxSize, int timeout,
                          const std::string& password, int fullCoverage)
{
	ipStr_ = ipStr;
	port_ = port;
	minSize_ = minSize;
	maxSize_ = maxSize;
	timeout_ = timeout;
	password_ = password;
	fullCoverage_ = fullCoverage;

	RedisClient client(NULL);
	client.init(ipStr_, port_, timeout_, password_);
	if (!client.connect())
	{
		return -1;
	}

	redisContext* context = client.getContext();
	if (context == NULL)
	{
		return -1;
	}

	// 判断Redis服务是否为Cluster
	bool bRet = clusterEnabled(context);
	if (!bRet)
	{
		return -1;
	}

	// 判断集群状态是否 "ok"
	bRet = clusterInfo(context);
	if (!bRet)
	{
		return -1;
	}

	// 获取集群的各节点信息
	bRet = getClusterNodesInfo(context);
	if (!bRet)
	{
		return -1;
	}

	// 连接各个集群结点
	for (size_t i = 0; i < nodes_.size(); ++i)
	{
		bRet = connectClusterNode(nodes_[i]);
		if (!bRet)
		{
			return -1;
		}
	}

	return 0;
}

int RedisClusterMgr::init2(const std::string& brokers,
                           int minSize, int maxSize, int timeout,
                           const std::string& password, int fullCoverage)
{
	brokers_ = brokers;
	minSize_ = minSize;
	maxSize_ = maxSize;
	timeout_ = timeout;
	password_ = password;
	fullCoverage_ = fullCoverage;

	std::vector<std::string> result;
	split(brokers_, ",", &result);
	if (result.size() == 0)
	{
		return -1;
	}

	for (size_t i = 0; i < result.size(); i++)
	{
		std::string ipStr, portStr;
		std::string::size_type index = result[i].find(":", 0);
		if (index == std::string::npos)
		{
			// 配置节点的数据格式有问题，直接返回出错
			return -1;
		}
		else
		{
			ipStr = result[i].substr(0, index);
			portStr = result[i].substr(index + 1, result[i].size() - index - 1);
		}

		RedisClient client(NULL);
		client.init(ipStr, atoi(portStr.c_str()), timeout_, password_);
		if (!client.connect())
		{
			// 如果连接失败，表示集群节点不可用，尝试下一个节点
			continue;
		}

		redisContext* context = client.getContext();
		if (context == NULL)
		{
			return -1;
		}

		// 判断Redis服务是否为Cluster
		bool bRet = clusterEnabled(context);
		if (!bRet)
		{
			return -1;
		}

		// 判断集群状态是否 "ok"
		bRet = clusterInfo(context);
		if (!bRet)
		{
			return -1;
		}

		// 获取集群的各结点信息
		bRet = getClusterNodesInfo(context);
		if (!bRet)
		{
			return -1;
		}

		// 连接各个集群结点
		for (size_t i = 0; i < nodes_.size(); ++i)
		{
			bRet = connectClusterNode(nodes_[i]);
			if (!bRet)
			{
				return -1;
			}
		}
		return 0;
	}

	// 如果返回填的节点都是连接失败，则直接返回失败
	return -1;
}

void RedisClusterMgr::split(const std::string& str,
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

bool RedisClusterMgr::clusterEnabled(redisContext* context)
{
	redisReply* reply = static_cast<redisReply* >(redisCommand(context, "INFO"));
	if ((NULL == reply) || (NULL == reply->str))
	{
		if (reply)
		{
			freeReplyObject(reply);
		}
		return false;
	}
	char* p = strstr(reply->str, "cluster_enabled:");
	bool bRet = ((p != NULL) && (0 == strncmp(p + strlen("cluster_enabled:"), "1", 1)));
	freeReplyObject(reply);
	return bRet;
}

bool RedisClusterMgr::clusterInfo(redisContext* context)
{
	redisReply* reply = static_cast<redisReply* >(redisCommand(context, "CLUSTER INFO"));
	if ((NULL == reply) || (NULL == reply->str))
	{
		if (reply)
		{
			freeReplyObject(reply);
		}
		return false;
	}

	char* p = strstr(reply->str, ":");
	bool bRet = (0 == strncmp(p + 1, "ok", 2));
	freeReplyObject(reply);
	return bRet;
}

bool RedisClusterMgr::getClusterNodesInfo(redisContext* context)
{
	redisReply* reply = static_cast<redisReply* >(redisCommand(context, "CLUSTER NODES"));
	if ((NULL == reply) || (NULL == reply->str))
	{
		if (reply)
		{
			freeReplyObject(reply);
		}
		return false;
	}

	std::vector<std::string> vLines;
	// str2Vec(reply->str, vLines, "\n");
	split(reply->str, "\n", &vLines);
	nodes_.clear();

	for (size_t i = 0; i < vLines.size(); ++i)
	{
		NodeInfo node;
		std::vector<std::string> nodeInfo;
		// str2Vec(vLines[i].c_str(), nodeInfo, " ");
		split(vLines[i], " ", &nodeInfo);
		if (nodeInfo.size() < 8)
		{
			return false;
		}

		// 过滤掉slave节点和失效的master节点
		if ((NULL == strstr(nodeInfo[2].c_str(), "master")) ||
		        (NULL != strstr(nodeInfo[2].c_str(), "master,fail")) ||
		        (NULL != strstr(nodeInfo[2].c_str(), "myself,master,fail")))
		{
			continue;
		}

		// 如果是Master结点一定有9个或9个以上元素
		/**
		eb49bea0a910d757bb7644136ad0e36b55605c03 192.169.0.60:7000 myself,master - 0 0 1 connected 0-5461 10920-10922 16383
		d125d927fb2dbb0123583dbe9cece09206474dcf 192.169.0.61:7000 master - 0 1515656147599 2 connected 5462-10919
		e334247b46da5d296fcf98fedb4f221854775759 192.169.0.60:7001 slave d125d927fb2dbb0123583dbe9cece09206474dcf 0 1515656143592 4 connected
		2bc89c7bd354c4ecfb9cc9630a24c2944194d121 192.169.0.62:7000 master - 0 1515656145596 3 connected 10923-16382
		12aad1513d0dcaf4be49e60d7a17f6a866abbe9e 192.169.0.61:7001 slave eb49bea0a910d757bb7644136ad0e36b55605c03 0 1515656146597 5 connected
		baca54ba0cc9ce610c97ec362a9c8b6aa72f70a4 192.169.0.62:7001 slave 2bc89c7bd354c4ecfb9cc9630a24c2944194d121 0 1515656144593 6 connected
		 */

		if (nodeInfo.size() < 9)
		{
			return false;
		}

		node.id = nodeInfo[0];
		node.ParseNodeString(nodeInfo[1]);

		for (size_t index = 8; index < nodeInfo.size(); index++)
		{
			node.ParseSlotString(nodeInfo[index]);
		}

		nodes_.push_back(node);
	}
	return true;
}

bool RedisClusterMgr::str2Vec(const char* src, std::vector<std::string>& dest, const char* delim)
{
	if (NULL == src || delim == NULL)
	{
		return false;
	}

	char* tmp  = const_cast<char* >(src);
	char* curr = strtok(tmp, delim);
	while (curr)
	{
		dest.push_back(curr);
		curr = strtok(NULL, delim);
	}
	return true;
}

bool RedisClusterMgr::connectClusterNode(const NodeInfo& node)
{
	auto it = nodesStatus_.find(node.id);
	if (it != nodesStatus_.end())
	{
		// 节点已经在Map里面可以不再生成该节点
		if (it->second == true)
		{
			return true;
		}
	}

	printf("new redis connect nodeid=%s host=%s port=%d", node.id.c_str(), node.ipStr.c_str(), node.port);
	RedisClusterNodePtr clusterNode(new RedisClusterNode());
	if (!clusterNode)
	{
		return false;
	}

	int bRet = clusterNode->init(node, minSize_, maxSize_, timeout_, password_);
	if (!bRet)
	{
		return false;
	}
	
	const std::vector<SlotsInfo>& slotsVec = node.getSlotsVec();
	for (size_t i = 0; i < slotsVec.size(); i++)
	{
		for (uint16_t pos = slotsVec[i].slotStart; pos <= slotsVec[i].slotEnd; pos++)
		{
			//fix bug map insert and [] diff
			//slotMap_.insert(std::make_pair(pos, clusterNode));
			slotMap_[pos] = clusterNode;
		}
	}

	printf("success connect redis nodeid=%s", node.id.c_str());
	nodesStatus_.insert(std::make_pair(node.id, true));
	return true;
}

RedisClient* RedisClusterMgr::findNodeConnection(const std::string& key)
{
	// 检测状态是否可用，如果不可用则不能取到Redis连接
	if (!bStatus_ && fullCoverage_)
	{
		printf("redis cluster status fail will reconnect");
		//明确配置必须全覆盖 才可用(可能导致异常无法恢复)
		return NULL;
	}

	uint16_t slotIndex = getKeySlotIndex(key.c_str());
	return getConnection(slotIndex);
}

RedisClient* RedisClusterMgr::getConnection(uint16_t slotIndex)
{
	std::map<uint16_t, RedisClusterNodePtr>::iterator it = slotMap_.find(slotIndex);
	if (it == slotMap_.end() || !(it->second))
	{
		return NULL;
	}

	if (it->second->getStatus() == false)
	{
		bStatus_ = false;

		{
			// 初始化互斥
			//common::MutexLockGuard lock(initMtx_);
			std::lock_guard<std::mutex> lock_(m_lock);
			if (bStatus_ == false)
			{
				nodesStatus_.erase(it->second->getNodeId());
				int ret = init2(brokers_, minSize_, maxSize_, timeout_, password_);
				if (ret != CONNECT_SUCCESS)
				{
					return NULL;
				}

				//重连恢复之后 再次获取本次请求的连接
				bStatus_ = true;
			}
		}

		
		auto itnew = slotMap_.find(slotIndex);
		if (itnew != slotMap_.end() && (itnew->second))
		{
			return itnew->second->getRedisClient();
		}
	}

	return it->second->getRedisClient();
}

uint16_t RedisClusterMgr::getKeySlotIndex(const char* key)
{
	if (NULL != key)
	{
		return keyHashSlot(key, strlen(key));
	}
	return 0;
}

uint16_t RedisClusterMgr::keyHashSlot(const char* key, size_t len)
{
	size_t s, e;
	for (s = 0; s < len; s++)
		if (key[s] == '{')
			break;

	if (s == len)
		return crc16(key, len) & 0x3FFF;

	for (e = s + 1; e < len; e++)
		if (key[e] == '}')
			break;

	if (e == len || e == s + 1)
		return crc16(key, len) & 0x3FFF;

	return crc16(key + s + 1, e - s - 1) & 0x3FFF;
}

// const std::string& RedisClusterMgr::findNodeId(uint16_t slotIndex)
// {
// 	for (size_t i = 0; i < nodes_.size(); ++i)
// 	{
// 		// const NodeInfo& node = nodes_[i];
// 		if (nodes_[i].CheckSlot(slotIndex))
// 		{
// 			return nodes_[i].id;
// 		}
// 	}
// 	return nullStr;
// }

