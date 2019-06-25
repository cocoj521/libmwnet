#ifndef MWNET_MT_USERONLINELIST_H
#define MWNET_MT_USERONLINELIST_H

#include <stdint.h>
#include <stdio.h>
#include <boost/any.hpp>
#include <string>
#include <memory>
#include <mutex>
#include <map>
#include <mwnet_mt/util/MWTimestamp.h>


namespace MWNET_MT
{
namespace SERVER
{

//用户信息结构体
//useruuid_:指的是业务层可以惟一标识一个用户身份的ID
//userinfo_:定义为any类型,用于保存任意类型的用户信息.可以将业务层的用户信息的智能指针存入在线列表
struct tUserInfo
{
	uint64_t useruuid_;		//用户ID
	boost::any userinfo_;	//用户对象
	tUserInfo()
    {
		useruuid_ = 0;
		userinfo_ = nullptr;
	}
};

typedef std::shared_ptr<tUserInfo> UserInfoPtr;

// 计数操作 +/-
enum enumCntOpt
{
	DEC_CNT = -1,
	INT_CNT =  1
};
	
// 统计计数，耗时 等等 信息结构
struct tStatistics
{
	size_t waitsndcnt_;					//待发数
	size_t waitrspcnt_;					//待回应数
	size_t slidewndcnt_;				//已占滑动窗口数
	size_t totalrecvcnt_;				//总接收量
	size_t totalsendcnt_;				//总发送量
	MWTIMESTAMP::Timestamp last_active_;//最后一次活跃时间
	// int64_t timeconsuming_;
	// 各种耗时等等
	tStatistics()
    {
		waitsndcnt_ 	= 0;
		waitrspcnt_ 	= 0;
		slidewndcnt_	= 0;
		totalrecvcnt_ 	= 0;
		totalsendcnt_ 	= 0;
		last_active_ 	= MWTIMESTAMP::Timestamp::Now();
	}
};
typedef std::shared_ptr<tStatistics> StatisticsPtr;
typedef std::vector<StatisticsPtr> VecStatisticsPtr;

// 连接信息结构
struct tConnInfo
{
	uint64_t conn_uuid_;				//连接ID
	boost::any conn_;					//连接对象
	std::string ipport_;				//客户端的ip和port
	StatisticsPtr pConnStat_;			//连接统计信息
	VecStatisticsPtr vConnCmdStat_;		//不同命令的统计信息
	tConnInfo()
    {
		conn_uuid_ 	= 0;
		conn_ 		= nullptr;
		ipport_		= "";
		pConnStat_ 	= nullptr;
	}
};
typedef std::shared_ptr<tConnInfo> ConnInfoPtr;
typedef std::weak_ptr<tConnInfo> weakConnInfoPtr;

struct tMinSlideWndConn
{
	size_t minwnd_;
	weakConnInfoPtr pConn_;
	tMinSlideWndConn()
    {
		minwnd_ = 0;
	}
};
typedef std::vector<tMinSlideWndConn> VecMinSlideWndConn;

// 用户在线信息结构
struct tUserOnlineInfo
{
	UserInfoPtr  pUserInfo_;									//用户信息
	StatisticsPtr pUserStat_;									//用户统计信息
	VecStatisticsPtr vUserCmdStat_;								//用户每种不同命令的总统计计数
	VecMinSlideWndConn vMinWndConn;								//滑动窗口最小的连接（按命令区分）
	std::map<uint64_t/*conn_uuid*/, ConnInfoPtr> mapConnInfo_;	//每个连接上不同命令的统计计数
	tUserOnlineInfo()
    {
		pUserInfo_ 	= nullptr;
		pUserStat_ 	= nullptr;
	}
};
typedef std::shared_ptr<tUserOnlineInfo> UserOnlineInfoPtr;

class UserOnlineList
{
public:
	UserOnlineList(int maxcmdcnt/*最多有多少个命令*/);
	~UserOnlineList();
public:
	// 加入在线列表（必须在给用户发了回应包以后，并且收到网络层sendok的回调后，才可以调用）
	// useruuid:指的是业务层可以惟一标识一个用户身份的ID
	// userinfo:定义为any类型,用于保存任意类型的用户信息.可以将业务层的用户信息的智能指针存入在线列表
	// useruuid,userinfo需要组成结构体在连接建立后，调用settcpsession函数，将其保存
	// conn_uuid,conn为网络层用于标识一个连接身份的两个变量
	// 返回值,0:成功,非0:失败
	int AddToOnlineList(uint64_t useruuid, const boost::any& userinfo, uint64_t conn_uuid, const boost::any& conn, const std::string& ipport);

	// 从在线列表中清除
	// 连接断开时会回调session信息，可取出useruuid，完成在线列表清除
	// 返回值,0:成功,非0:失败
	int DelFromOnlineList(uint64_t useruuid, uint64_t conn_uuid, const boost::any& conn);

	// 获取最佳连接
	// 如果一个用户有多个连接，优先选择:网络层待发缓冲最小的和业务层等待回应最小的连接
	// 返回值,0:成功,非0:失败                   conn_uuid,conn为返回的最佳连接
	int GetBestConn(uint64_t useruuid, uint64_t& conn_uuid, boost::any& conn, int cmd);

	// 获取用户总的在线连接数量
	// 返回值,用户总的在线连接数量
	size_t GetUserOnlineConnCnt(uint64_t useruuid);

	// 更新总的接收量(只增不减)
	void UpdateTotalRecvCnt(uint64_t useruuid, uint64_t conn_uuid, const boost::any& conn, int cmd);

	// 更新总的发收量(只增不减)
	void UpdateTotalSendCnt(uint64_t useruuid, uint64_t conn_uuid, const boost::any& conn, int cmd);
	
	// 更新网络层待发送的数量（调用sendmsgwithtcpserver成功后计数inc，onsendok/onsenderr后计数dec）
	void UpdateWaitSndCnt(uint64_t useruuid, uint64_t conn_uuid, const boost::any& conn, int cmd, enumCntOpt opt);

	// 获取网络层待发送的数量 connuuid填0表示所有连接，cmd填maxcmdcnt表示所有命令
	// 返回值,网络层待发送的数量
	size_t GetWaitSndCnt(uint64_t useruuid, uint64_t conn_uuid, const boost::any& conn, int cmd);

	// 更新业务数据待回应的数量（onsendok返回后计数inc，onrecvmsg返回后计数dec）
	void UpdateWaitRspCnt(uint64_t useruuid, uint64_t conn_uuid, const boost::any& conn, int cmd, enumCntOpt opt);

	// 获取业务数据待回应的数量 connuuid填0表示所有连接，cmd填maxcmdcnt表示所有命令
	// 返回值,业务数据待回应的数量
	size_t GetWaitRspCnt(uint64_t useruuid, uint64_t conn_uuid, const boost::any& conn, int cmd);

	// 更新已占用窗口计数（调用sendmsgwithtcpserver之前inc,若调用后败再dec,若调用成功,等待回调,onsenderr回调/onrecvmsg回调时dec）
	void UpdateSlideWndCnt(uint64_t useruuid, uint64_t conn_uuid, const boost::any& conn, int cmd, enumCntOpt opt);
	
	// 获取已占用窗口数数量 connuuid填0表示所有连接，cmd填maxcmdcnt表示所有命令
	// 返回值,已占用窗口数数量
	size_t GetSlideWndCnt(uint64_t useruuid, uint64_t conn_uuid, const boost::any& conn, int cmd);
private:
	int maxcmdcnt_;//最多支持的命令字数量，用来控制计数数组下标
	
	std::mutex mutex_;
	std::map<uint64_t/*useruuid*/, UserOnlineInfoPtr> mapUserOnlineInfo_; //用户在线列表
};
}
}
#endif  // MWNET_MT_USERONLINELIST_H

