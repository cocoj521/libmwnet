#include "UserOnlineList.h"
#include <memory>
#include <stdio.h>
#include <unistd.h>
#include <vector>
#include <list>
#include <atomic>
#include <mutex>

namespace MWNET_MT
{
namespace SERVER
{

#ifndef CHECK_CMD_VALUE_INVALID
#define CHECK_CMD_VALUE_INVALID(cmd,maxcmdcnt) {\
			(cmd<0||cmd>=maxcmdcnt)?cmd=0:1;\
		}
#endif

UserOnlineList::UserOnlineList(int maxcmdcnt/*最多有多少个命令*/)
	: maxcmdcnt_(maxcmdcnt)
{
}

UserOnlineList::~UserOnlineList()
{
}

// 加入在线列表（必须在给用户发了回应包以后，并且收到网络层sendok的回调后，才可以调用）
// useruuid:指的是业务层可以惟一标识一个用户身份的ID
// userinfo:定义为any类型,用于保存任意类型的用户信息.可以将业务层的用户信息的智能指针存入在线列表
// useruuid,userinfo需要组成结构体在连接建立后，调用settcpsession函数，将其保存
// conn_uuid,conn为网络层用于标识一个连接身份的两个变量
// 返回值,0:成功,非0:失败
int UserOnlineList::AddToOnlineList(uint64_t useruuid, const boost::any& userinfo, uint64_t conn_uuid, const boost::any& conn, const std::string& ipport)
{
	int ret = 0;

	MWTIMESTAMP::Timestamp tNow = MWTIMESTAMP::Timestamp::Now();

	std::lock_guard<std::mutex> lock(mutex_);
	std::map<uint64_t, UserOnlineInfoPtr>::iterator itUser = mapUserOnlineInfo_.find(useruuid);
	//用户,存在更新
	if (itUser != mapUserOnlineInfo_.end())
	{		
		itUser->second->pUserInfo_->useruuid_ 	 = useruuid;
		itUser->second->pUserInfo_->userinfo_ 	 = userinfo;
		itUser->second->pUserStat_->last_active_ = tNow;

		std::map<uint64_t, ConnInfoPtr>::iterator itConn = itUser->second->mapConnInfo_.find(conn_uuid);
		//连接,存在更新
		if (itConn != itUser->second->mapConnInfo_.end())
		{
			itConn->second->conn_uuid_ 					= conn_uuid;
			itConn->second->conn_ 	  					= conn;
			itConn->second->pConnStat_->last_active_	= tNow;
		}
		//连接,不存在插入
		else
		{
			ConnInfoPtr pConnInfo(new tConnInfo);
			pConnInfo->conn_uuid_	= conn_uuid;
			pConnInfo->conn_		= conn;
			pConnInfo->ipport_		= ipport;
			pConnInfo->pConnStat_.reset(new tStatistics);
			pConnInfo->pConnStat_->last_active_ = tNow;
			pConnInfo->vConnCmdStat_.resize(maxcmdcnt_);
			for (int i = 0; i < maxcmdcnt_; ++i)
			{
				pConnInfo->vConnCmdStat_[i].reset(new tStatistics);
			}
			itUser->second->mapConnInfo_.insert(std::make_pair(conn_uuid, pConnInfo));
		}
	}
	//用户,不存在插入
	else
	{
		UserOnlineInfoPtr pUserOnlineInfo(new tUserOnlineInfo);
		UserInfoPtr pUserInfo(new tUserInfo);
		pUserInfo->useruuid_ 		= useruuid;
		pUserInfo->userinfo_ 		= userinfo;
		pUserOnlineInfo->pUserInfo_ = pUserInfo;
		pUserOnlineInfo->pUserStat_.reset(new tStatistics);
		pUserOnlineInfo->pUserStat_->last_active_ = tNow;
		pUserOnlineInfo->vMinWndConn.resize(maxcmdcnt_);
		pUserOnlineInfo->vUserCmdStat_.resize(maxcmdcnt_);
		for (int i = 0; i < maxcmdcnt_; ++i)
		{
			pUserOnlineInfo->vUserCmdStat_[i].reset(new tStatistics);
		}
		ConnInfoPtr pConnInfo(new tConnInfo);
		pConnInfo->conn_uuid_	= conn_uuid;
		pConnInfo->conn_		= conn;
		pConnInfo->ipport_		= ipport;
		pConnInfo->pConnStat_.reset(new tStatistics);
		pConnInfo->pConnStat_->last_active_ = tNow;
		pConnInfo->vConnCmdStat_.resize(maxcmdcnt_);
		for (int j = 0; j < maxcmdcnt_; ++j)
		{
			pConnInfo->vConnCmdStat_[j].reset(new tStatistics);
		}
		pUserOnlineInfo->mapConnInfo_.insert(std::make_pair(conn_uuid, pConnInfo));

		mapUserOnlineInfo_.insert(std::make_pair(useruuid, pUserOnlineInfo));		
	}
	
	return ret;
}

// 从在线列表中清除
// 连接断开时会回调session信息，可取出useruuid，完成在线列表清除
// 返回值,0:成功,非0:失败
int UserOnlineList::DelFromOnlineList(uint64_t useruuid, uint64_t conn_uuid, const boost::any& conn)
{
	int ret = 0;
	
	std::lock_guard<std::mutex> lock(mutex_);
	std::map<uint64_t, UserOnlineInfoPtr>::iterator itUser = mapUserOnlineInfo_.find(useruuid);
	if (itUser != mapUserOnlineInfo_.end())
	{
		std::map<uint64_t, ConnInfoPtr>::iterator itConn = itUser->second->mapConnInfo_.find(conn_uuid);
		if (itConn != itUser->second->mapConnInfo_.end())
		{
			itUser->second->mapConnInfo_.erase(itConn);
		}
	}
		
	return ret;
}

// 获取最佳连接
// 如果一个用户有多个连接，优先选择:网络层待发缓冲最小的和业务层等待回应最小的连接
// 返回值,0:成功,1:无可用连接,其他:其他失败					conn_uuid,conn为返回的最佳连接
int UserOnlineList::GetBestConn(uint64_t useruuid, uint64_t& conn_uuid, boost::any& conn, int cmd)
{
	int ret = 1;

	CHECK_CMD_VALUE_INVALID(cmd, maxcmdcnt_);
	
	std::lock_guard<std::mutex> lock(mutex_);
	std::map<uint64_t, UserOnlineInfoPtr>::iterator itUser = mapUserOnlineInfo_.find(useruuid);
	if (itUser != mapUserOnlineInfo_.end())
	{
		//没有可用连接
		if (!itUser->second->mapConnInfo_.empty())
		{
			ConnInfoPtr pConnInfo(itUser->second->vMinWndConn[cmd].pConn_.lock());
			//如果没有连接，取排序第一位的连接
			if (nullptr == pConnInfo)
			{
				std::map<uint64_t, ConnInfoPtr>::iterator itConn = itUser->second->mapConnInfo_.begin();
				itUser->second->vMinWndConn[cmd].minwnd_ = itConn->second->vConnCmdStat_[cmd]->slidewndcnt_;
				itUser->second->vMinWndConn[cmd].pConn_  = itConn->second;
				conn_uuid = itConn->second->conn_uuid_;
				conn 	  = itConn->second->conn_;
			}
			else
			{
				conn_uuid = pConnInfo->conn_uuid_;
				conn 	  = pConnInfo->conn_;
			}
			ret = 0;	
		}
	}
	
	return ret;
}

// 获取用户总的在线连接数量
// 返回值,用户总的在线连接数量
size_t UserOnlineList::GetUserOnlineConnCnt(uint64_t useruuid)
{
	size_t cnt = 0;

	std::lock_guard<std::mutex> lock(mutex_);
	std::map<uint64_t, UserOnlineInfoPtr>::iterator itUser = mapUserOnlineInfo_.find(useruuid);
	if (itUser != mapUserOnlineInfo_.end())
	{
		cnt = itUser->second->mapConnInfo_.size();
	}
	
	return cnt;
}

// 更新总的接收量(只增不减)
void UserOnlineList::UpdateTotalRecvCnt(uint64_t useruuid, uint64_t conn_uuid, const boost::any& conn, int cmd)
{
	CHECK_CMD_VALUE_INVALID(cmd, maxcmdcnt_);	

	MWTIMESTAMP::Timestamp tNow = MWTIMESTAMP::Timestamp::Now();

	std::lock_guard<std::mutex> lock(mutex_);
	std::map<uint64_t, UserOnlineInfoPtr>::iterator itUser = mapUserOnlineInfo_.find(useruuid);
	if (itUser != mapUserOnlineInfo_.end())
	{
		++itUser->second->pUserStat_->totalrecvcnt_;
		itUser->second->pUserStat_->last_active_ = tNow;//只有接收到包才认为是活跃
		++itUser->second->vUserCmdStat_[cmd]->totalrecvcnt_;
		itUser->second->vUserCmdStat_[cmd]->last_active_ = tNow;//只有接收到包才认为是活跃
		std::map<uint64_t, ConnInfoPtr>::iterator itConn = itUser->second->mapConnInfo_.find(conn_uuid);
		if (itConn != itUser->second->mapConnInfo_.end())
		{
			++itConn->second->pConnStat_->totalrecvcnt_;
			itConn->second->pConnStat_->last_active_ = tNow;//只有接收到包才认为是活跃
			++itConn->second->vConnCmdStat_[cmd]->totalrecvcnt_;
			itConn->second->vConnCmdStat_[cmd]->last_active_ = tNow;//只有接收到包才认为是活跃
		}
	}
}

// 获取总接收量 connuuid填0表示所有连接，cmd填maxcmdcnt表示所有命令
// 返回值,满足条件的总接收量
size_t UserOnlineList::GetTotalRecvCnt(uint64_t useruuid, uint64_t conn_uuid, const boost::any& conn, int cmd)
{
	size_t cnt = 0;

	CHECK_CMD_VALUE_INVALID(cmd, maxcmdcnt_);
	
	std::lock_guard<std::mutex> lock(mutex_);
	std::map<uint64_t, UserOnlineInfoPtr>::iterator itUser = mapUserOnlineInfo_.find(useruuid);
	if (itUser != mapUserOnlineInfo_.end())
	{
		if (0 == conn_uuid)
		{
			if (maxcmdcnt_ == cmd)
			{
				cnt = itUser->second->pUserStat_->totalrecvcnt_;
			}
			else
			{
				cnt = itUser->second->vUserCmdStat_[cmd]->totalrecvcnt_;
			}
		}
		else
		{
			std::map<uint64_t, ConnInfoPtr>::iterator itConn = itUser->second->mapConnInfo_.find(conn_uuid);
			if (itConn != itUser->second->mapConnInfo_.end())
			{
				if (maxcmdcnt_ == cmd)
				{
					cnt = itConn->second->pConnStat_->totalrecvcnt_;
				}
				else
				{
					cnt = itConn->second->vConnCmdStat_[cmd]->totalrecvcnt_;
				}
			}
		}		
	}

	return cnt;
}

// 更新总的发收量(只增不减)
void UserOnlineList::UpdateTotalSendCnt(uint64_t useruuid, uint64_t conn_uuid, const boost::any& conn, int cmd)
{
	CHECK_CMD_VALUE_INVALID(cmd, maxcmdcnt_);

	std::lock_guard<std::mutex> lock(mutex_);
	std::map<uint64_t, UserOnlineInfoPtr>::iterator itUser = mapUserOnlineInfo_.find(useruuid);
	if (itUser != mapUserOnlineInfo_.end())
	{
		++itUser->second->pUserStat_->totalsendcnt_;
		++itUser->second->vUserCmdStat_[cmd]->totalsendcnt_;
		std::map<uint64_t, ConnInfoPtr>::iterator itConn = itUser->second->mapConnInfo_.find(conn_uuid);
		if (itConn != itUser->second->mapConnInfo_.end())
		{
			++itConn->second->pConnStat_->totalsendcnt_;
			++itConn->second->vConnCmdStat_[cmd]->totalsendcnt_;
		}
	}
}

// 获取总发送量 connuuid填0表示所有连接，cmd填maxcmdcnt表示所有命令
// 返回值,满足条件的总接收量
size_t UserOnlineList::GetTotalSendCnt(uint64_t useruuid, uint64_t conn_uuid, const boost::any& conn, int cmd)
{
	size_t cnt = 0;
	
	CHECK_CMD_VALUE_INVALID(cmd, maxcmdcnt_);
	
	std::lock_guard<std::mutex> lock(mutex_);
	std::map<uint64_t, UserOnlineInfoPtr>::iterator itUser = mapUserOnlineInfo_.find(useruuid);
	if (itUser != mapUserOnlineInfo_.end())
	{
		if (0 == conn_uuid)
		{
			if (maxcmdcnt_ == cmd)
			{
				cnt = itUser->second->pUserStat_->totalsendcnt_;
			}
			else
			{
				cnt = itUser->second->vUserCmdStat_[cmd]->totalsendcnt_;
			}
		}
		else
		{
			std::map<uint64_t, ConnInfoPtr>::iterator itConn = itUser->second->mapConnInfo_.find(conn_uuid);
			if (itConn != itUser->second->mapConnInfo_.end())
			{
				if (maxcmdcnt_ == cmd)
				{
					cnt = itConn->second->pConnStat_->totalsendcnt_;
				}
				else
				{
					cnt = itConn->second->vConnCmdStat_[cmd]->totalsendcnt_;
				}
			}
		}		
	}

	return cnt;
}

// 更新网络层待发送的数量（调用sendmsgwithtcpserver成功后计数inc，onsendok/onsenderr后计数dec）
void UserOnlineList::UpdateWaitSndCnt(uint64_t useruuid, uint64_t conn_uuid, const boost::any& conn, int cmd, enumCntOpt opt)
{
	CHECK_CMD_VALUE_INVALID(cmd, maxcmdcnt_);

	std::lock_guard<std::mutex> lock(mutex_);
	std::map<uint64_t, UserOnlineInfoPtr>::iterator itUser = mapUserOnlineInfo_.find(useruuid);
	if (itUser != mapUserOnlineInfo_.end())
	{
		itUser->second->pUserStat_->waitsndcnt_ += opt;
		itUser->second->vUserCmdStat_[cmd]->waitsndcnt_ += opt;
		std::map<uint64_t, ConnInfoPtr>::iterator itConn = itUser->second->mapConnInfo_.find(conn_uuid);
		if (itConn != itUser->second->mapConnInfo_.end())
		{
			itConn->second->pConnStat_->waitsndcnt_ += opt;
			itConn->second->vConnCmdStat_[cmd]->waitsndcnt_ += opt;
		}
	}
}

// 获取网络层待发送的数量 connuuid填0表示所有连接，cmd填maxcmdcnt表示所有命令
// 返回值,网络层待发送的数量
size_t UserOnlineList::GetWaitSndCnt(uint64_t useruuid, uint64_t conn_uuid, const boost::any& conn, int cmd)
{
	size_t cnt = 0;
	
	CHECK_CMD_VALUE_INVALID(cmd, maxcmdcnt_);

	std::lock_guard<std::mutex> lock(mutex_);
	std::map<uint64_t, UserOnlineInfoPtr>::iterator itUser = mapUserOnlineInfo_.find(useruuid);
	if (itUser != mapUserOnlineInfo_.end())
	{
		if (0 == conn_uuid)
		{
			if (maxcmdcnt_ == cmd)
			{
				cnt = itUser->second->pUserStat_->waitsndcnt_;
			}
			else
			{
				cnt = itUser->second->vUserCmdStat_[cmd]->waitsndcnt_;
			}
		}
		else
		{
			std::map<uint64_t, ConnInfoPtr>::iterator itConn = itUser->second->mapConnInfo_.find(conn_uuid);
			if (itConn != itUser->second->mapConnInfo_.end())
			{
				if (maxcmdcnt_ == cmd)
				{
					cnt = itConn->second->pConnStat_->waitsndcnt_;
				}
				else
				{
					cnt = itConn->second->vConnCmdStat_[cmd]->waitsndcnt_;
				}
			}
		}		
	}

	return cnt;
}

// 更新业务数据待回应的数量（onsendok返回后计数inc，onrecvmsg返回后计数dec）
void UserOnlineList::UpdateWaitRspCnt(uint64_t useruuid, uint64_t conn_uuid, const boost::any& conn, int cmd, enumCntOpt opt)
{
	CHECK_CMD_VALUE_INVALID(cmd, maxcmdcnt_);

	std::lock_guard<std::mutex> lock(mutex_);
	std::map<uint64_t, UserOnlineInfoPtr>::iterator itUser = mapUserOnlineInfo_.find(useruuid);
	if (itUser != mapUserOnlineInfo_.end())
	{
		itUser->second->pUserStat_->waitrspcnt_ += opt;
		itUser->second->vUserCmdStat_[cmd]->waitrspcnt_ += opt;
		std::map<uint64_t, ConnInfoPtr>::iterator itConn = itUser->second->mapConnInfo_.find(conn_uuid);
		if (itConn != itUser->second->mapConnInfo_.end())
		{
			itConn->second->pConnStat_->waitrspcnt_ += opt;
			itConn->second->vConnCmdStat_[cmd]->waitrspcnt_ += opt;
		}
	}
}

// 获取业务数据待回应的数量 connuuid填0表示所有连接，cmd填maxcmdcnt表示所有命令
// 返回值,业务数据待回应的数量
size_t UserOnlineList::GetWaitRspCnt(uint64_t useruuid, uint64_t conn_uuid, const boost::any& conn, int cmd)
{
	size_t cnt = 0;

	CHECK_CMD_VALUE_INVALID(cmd, maxcmdcnt_);

	std::lock_guard<std::mutex> lock(mutex_);
	std::map<uint64_t, UserOnlineInfoPtr>::iterator itUser = mapUserOnlineInfo_.find(useruuid);
	if (itUser != mapUserOnlineInfo_.end())
	{
		if (0 == conn_uuid)
		{
			if (maxcmdcnt_ == cmd)
			{
				cnt = itUser->second->pUserStat_->waitrspcnt_;
			}
			else
			{
				cnt = itUser->second->vUserCmdStat_[cmd]->waitrspcnt_;
			}
		}
		else
		{
			std::map<uint64_t, ConnInfoPtr>::iterator itConn = itUser->second->mapConnInfo_.find(conn_uuid);
			if (itConn != itUser->second->mapConnInfo_.end())
			{
				if (maxcmdcnt_ == cmd)
				{
					cnt = itConn->second->pConnStat_->waitrspcnt_;
				}
				else
				{
					cnt = itConn->second->vConnCmdStat_[cmd]->waitrspcnt_;
				}
			}
		}		
	}

	return cnt;
}

// 更新已占用窗口计数（调用sendmsgwithtcpserver之前inc,若调用后败再dec,若调用成功,等待回调,onsenderr回调/onrecvmsg回调时dec）
void UserOnlineList::UpdateSlideWndCnt(uint64_t useruuid, uint64_t conn_uuid, const boost::any& conn, int cmd, enumCntOpt opt)
{
	CHECK_CMD_VALUE_INVALID(cmd, maxcmdcnt_);

	std::lock_guard<std::mutex> lock(mutex_);
	std::map<uint64_t, UserOnlineInfoPtr>::iterator itUser = mapUserOnlineInfo_.find(useruuid);
	if (itUser != mapUserOnlineInfo_.end())
	{
		itUser->second->pUserStat_->slidewndcnt_ += opt;
		itUser->second->vUserCmdStat_[cmd]->slidewndcnt_ += opt;
		std::map<uint64_t, ConnInfoPtr>::iterator itConn = itUser->second->mapConnInfo_.find(conn_uuid);
		if (itConn != itUser->second->mapConnInfo_.end())
		{
			itConn->second->pConnStat_->slidewndcnt_ += opt;
			size_t slidewnd = (itConn->second->vConnCmdStat_[cmd]->slidewndcnt_ += opt);
			//保存滑动窗口最小的连接
			if (slidewnd < itUser->second->vMinWndConn[cmd].minwnd_)
			{
				//更新最小窗口值
				itUser->second->vMinWndConn[cmd].minwnd_ = slidewnd;	
				//保存对应连接的弱指针
				itUser->second->vMinWndConn[cmd].pConn_  = itConn->second;
			}
		}		
	}
}

// 获取已占用窗口数数量 connuuid填0表示所有连接，cmd填maxcmdcnt表示所有命令
// 返回值,已占用窗口数数量
size_t UserOnlineList::GetSlideWndCnt(uint64_t useruuid, uint64_t conn_uuid, const boost::any& conn, int cmd)
{
	size_t cnt = 0;
	
	CHECK_CMD_VALUE_INVALID(cmd, maxcmdcnt_);

	std::lock_guard<std::mutex> lock(mutex_);
	std::map<uint64_t, UserOnlineInfoPtr>::iterator itUser = mapUserOnlineInfo_.find(useruuid);
	if (itUser != mapUserOnlineInfo_.end())
	{
		if (0 == conn_uuid)
		{
			if (maxcmdcnt_ == cmd)
			{
				cnt = itUser->second->pUserStat_->slidewndcnt_;
			}
			else
			{
				cnt = itUser->second->vUserCmdStat_[cmd]->slidewndcnt_;
			}
		}
		else
		{
			std::map<uint64_t, ConnInfoPtr>::iterator itConn = itUser->second->mapConnInfo_.find(conn_uuid);
			if (itConn != itUser->second->mapConnInfo_.end())
			{
				if (maxcmdcnt_ == cmd)
				{
					cnt = itConn->second->pConnStat_->slidewndcnt_;
				}
				else
				{
					cnt = itConn->second->vConnCmdStat_[cmd]->slidewndcnt_;
				}
			}
		}		
	}

	return cnt;
}

}
}
