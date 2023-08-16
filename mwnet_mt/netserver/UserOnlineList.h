#ifndef MWNET_MT_USERONLINELIST_H
#define MWNET_MT_USERONLINELIST_H

#include <stdint.h>
#include <stdio.h>
#include <boost/any.hpp>
#include <string>
#include <memory>
#include <mutex>
#include <map>
#include <vector>
#include <mwnet_mt/util/MWTimestamp.h>


namespace MWNET_MT
{
namespace SERVER
{

//�û���Ϣ�ṹ��
//useruuid_:ָ����ҵ������Ωһ��ʶһ���û���ݵ�ID
//userinfo_:����Ϊany����,���ڱ����������͵��û���Ϣ.���Խ�ҵ�����û���Ϣ������ָ����������б�
struct tUserInfo
{
	uint64_t useruuid_;		//�û�ID
	boost::any userinfo_;	//�û�����
	tUserInfo()
    {
		useruuid_ = 0;
		userinfo_ = nullptr;
	}
};

typedef std::shared_ptr<tUserInfo> UserInfoPtr;

// �������� +/-
enum enumCntOpt
{
	DEC_CNT = -1,
	INT_CNT =  1
};
	
// ͳ�Ƽ�������ʱ �ȵ� ��Ϣ�ṹ
struct tStatistics
{
	size_t waitsndcnt_;					//������
	size_t waitrspcnt_;					//����Ӧ��
	size_t slidewndcnt_;				//��ռ����������
	size_t totalrecvcnt_;				//�ܽ�����
	size_t totalsendcnt_;				//�ܷ�����
	MWTIMESTAMP::Timestamp last_active_;//���һ�λ�Ծʱ��
	// int64_t timeconsuming_;
	// ���ֺ�ʱ�ȵ�
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

// ������Ϣ�ṹ
struct tConnInfo
{
	uint64_t conn_uuid_;				//����ID
	boost::any conn_;					//���Ӷ���
	std::string ipport_;				//�ͻ��˵�ip��port
	StatisticsPtr pConnStat_;			//����ͳ����Ϣ
	VecStatisticsPtr vConnCmdStat_;		//��ͬ�����ͳ����Ϣ
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

// �û�������Ϣ�ṹ
struct tUserOnlineInfo
{
	UserInfoPtr  pUserInfo_;									//�û���Ϣ
	StatisticsPtr pUserStat_;									//�û�ͳ����Ϣ
	VecStatisticsPtr vUserCmdStat_;								//�û�ÿ�ֲ�ͬ�������ͳ�Ƽ���
	VecMinSlideWndConn vMinWndConn;								//����������С�����ӣ����������֣�
	std::map<uint64_t/*conn_uuid*/, ConnInfoPtr> mapConnInfo_;	//ÿ�������ϲ�ͬ�����ͳ�Ƽ���
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
	UserOnlineList(int maxcmdcnt/*����ж��ٸ�����*/);
	~UserOnlineList();
public:
	// ���������б������ڸ��û����˻�Ӧ���Ժ󣬲����յ������sendok�Ļص��󣬲ſ��Ե��ã�
	// useruuid:ָ����ҵ������Ωһ��ʶһ���û���ݵ�ID
	// userinfo:����Ϊany����,���ڱ����������͵��û���Ϣ.���Խ�ҵ�����û���Ϣ������ָ����������б�
	// useruuid,userinfo��Ҫ��ɽṹ�������ӽ����󣬵���settcpsession���������䱣��
	// conn_uuid,connΪ��������ڱ�ʶһ��������ݵ���������
	// ����ֵ,0:�ɹ�,��0:ʧ��
	int AddToOnlineList(uint64_t useruuid, const boost::any& userinfo, uint64_t conn_uuid, const boost::any& conn, const std::string& ipport);

	// �������б������
	// ���ӶϿ�ʱ��ص�session��Ϣ����ȡ��useruuid����������б����
	// ����ֵ,0:�ɹ�,��0:ʧ��
	int DelFromOnlineList(uint64_t useruuid, uint64_t conn_uuid, const boost::any& conn);

	// ��ȡ�������
	// ���һ���û��ж�����ӣ�����ѡ��:��������������С�ĺ�ҵ���ȴ���Ӧ��С������
	// ����ֵ,0:�ɹ�,��0:ʧ��                   conn_uuid,connΪ���ص��������
	int GetBestConn(uint64_t useruuid, uint64_t& conn_uuid, boost::any& conn, int cmd);

	// ��ȡ�û��ܵ�������������
	// ����ֵ,�û��ܵ�������������
	size_t GetUserOnlineConnCnt(uint64_t useruuid);

	// �����ܵĽ�����(ֻ������)
	void UpdateTotalRecvCnt(uint64_t useruuid, uint64_t conn_uuid, const boost::any& conn, int cmd);

	// ��ȡ�ܽ����� connuuid��0��ʾ�������ӣ�cmd��maxcmdcnt��ʾ��������
	// ����ֵ,�����������ܽ�����
	size_t GetTotalRecvCnt(uint64_t useruuid, uint64_t conn_uuid, const boost::any& conn, int cmd);
	
	// �����ܵķ�����(ֻ������)
	void UpdateTotalSendCnt(uint64_t useruuid, uint64_t conn_uuid, const boost::any& conn, int cmd);

	// ��ȡ�ܷ����� connuuid��0��ʾ�������ӣ�cmd��maxcmdcnt��ʾ��������
	// ����ֵ,�����������ܽ�����
	size_t GetTotalSendCnt(uint64_t useruuid, uint64_t conn_uuid, const boost::any& conn, int cmd);
	
	// �������������͵�����������sendmsgwithtcpserver�ɹ������inc��onsendok/onsenderr�����dec��
	void UpdateWaitSndCnt(uint64_t useruuid, uint64_t conn_uuid, const boost::any& conn, int cmd, enumCntOpt opt);

	// ��ȡ���������͵����� connuuid��0��ʾ�������ӣ�cmd��maxcmdcnt��ʾ��������
	// ����ֵ,���������͵�����
	size_t GetWaitSndCnt(uint64_t useruuid, uint64_t conn_uuid, const boost::any& conn, int cmd);

	// ����ҵ�����ݴ���Ӧ��������onsendok���غ����inc��onrecvmsg���غ����dec��
	void UpdateWaitRspCnt(uint64_t useruuid, uint64_t conn_uuid, const boost::any& conn, int cmd, enumCntOpt opt);

	// ��ȡҵ�����ݴ���Ӧ������ connuuid��0��ʾ�������ӣ�cmd��maxcmdcnt��ʾ��������
	// ����ֵ,ҵ�����ݴ���Ӧ������
	size_t GetWaitRspCnt(uint64_t useruuid, uint64_t conn_uuid, const boost::any& conn, int cmd);

	// ������ռ�ô��ڼ���������sendmsgwithtcpserver֮ǰinc,�����ú����dec,�����óɹ�,�ȴ��ص�,onsenderr�ص�/onrecvmsg�ص�ʱdec��
	void UpdateSlideWndCnt(uint64_t useruuid, uint64_t conn_uuid, const boost::any& conn, int cmd, enumCntOpt opt);
	
	// ��ȡ��ռ�ô��������� connuuid��0��ʾ�������ӣ�cmd��maxcmdcnt��ʾ��������
	// ����ֵ,��ռ�ô���������
	size_t GetSlideWndCnt(uint64_t useruuid, uint64_t conn_uuid, const boost::any& conn, int cmd);
private:
	int maxcmdcnt_;//���֧�ֵ��������������������Ƽ��������±�
	
	std::mutex mutex_;
	std::map<uint64_t/*useruuid*/, UserOnlineInfoPtr> mapUserOnlineInfo_; //�û������б�
};
}
}
#endif  // MWNET_MT_USERONLINELIST_H

