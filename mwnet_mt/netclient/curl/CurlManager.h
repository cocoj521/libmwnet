#ifndef __CURL_CURLMANAGER_H__
#define __CURL_CURLMANAGER_H__

#include "CurlRequest.h"
#include "CurlEvMgr.h"
#include <mwnet_mt/net/TimerId.h>
#include <mutex>
#include <map>
#include <list>

using namespace mwnet_mt;
using namespace mwnet_mt::net;
namespace curl
{

/**
 * curlm����Ĺ����ߣ��ɹ������򵥾��
 */
class CurlManager : noncopyable,
					public std::enable_shared_from_this<CurlManager>
{
public:
	enum Option
	{
		kCURLnossl = 0,
		kCURLssl   = 1,
	};

	/**
	 * [CurlManager ���캯��]
	 * @param  loop [epoll���¼�ѭ����]
	 */
	explicit CurlManager(EventLoop* loop);
	~CurlManager();

	/**
	 * [initialize ��ʼ������]
	 * @param opt [kCURLnossl����֧��HTTPS kCURLssl��֧��HTTPS]
	 */
	static void initialize(Option opt = kCURLnossl);
	static void uninitialize();
	
	// ��ȡһ���������
	static CurlRequestPtr getRequest(const std::string& url, bool bKeepAlive, int req_type, int http_ver);

	// ��������
	//0:�ɹ� 1:��δ��ʼ�� 2:���ͻ����� 3:�����ܵ���󲢷���
	int sendRequest(const CurlRequestPtr& request);

	// �����ж�����
	void forceCancelRequest(uint64_t req_uuid);
	// �����ж�����(�ڲ�����)
	void forceCancelRequestInner(const CurlRequestPtr& request);

	/**
	 * [���¼�����Ӧ����]
	 * @param fd [�¼����׽ӿ�]
	 */
	void onReadEventCallBack(int fd);
	
	/**
	 * [д�¼�����Ӧ����]
	 * @param fd [�¼����׽ӿ�]
	 */
	void onWriteEventCallBack(int fd);

	// �ж��¼�ѭ�����Ƿ���δ����¼�
	size_t isLoopRunning();
private:
	// ֪ͨ����
	void notifySendRequest();
	void onRecvWakeUpNotify();
	
	/**
	 * [check_multi_info ��ʵ�����Ƿ��Ѿ����]
	 */
	void check_multi_info();

	// ����Ƿ�����Ҫ���͵�����
	void checkNeedSendRequest();

	// ��������,����������ǻ������û���ֱ���ͷ�
	void recycleRequest(const CurlRequestPtr& request);

	// ��������	
	void sendRequestInLoop(const CurlRequestPtr& request);

	// �������Ƴ�������
	void removeMultiHandle(const CurlRequestPtr& request);

	// ����curl_multi_info_read���ص���Ӧ
	void afterRequestDone(const CurlRequestPtr& request);
	
	// �ڲ���ʱ����ʱ��Ӧ����
	void innerTimerTimeOut(uint64_t req_uuid);

	// curlm socket�¼��ص�CURLMOPT_SOCKETFUNCTION CURL_POLL_IN/OUT/INOUT/REMOVE
	static int curlmSocketOptCb(CURL* c, int fd, int what, void* userp, void* socketp);
	int curlmSocketOptCbInLoop(CURL* c, int fd, int what, void* socketp);
	void addEvLoop(void* p, int fd, int what);
	void optEvLoop(void* p, int fd, int what);
	void delEvLoop(void* p, int fd, int what);
	
	// curlm��ʱ�ص�����CURLMOPT_TIMERFUNCTION
	static int curlmTimerCb(CURLM* curlm, long ms, void* userp);
	void curlmTimerCbInLoop(long ms);
	void createTimer(long ms);
	void cancelTimer();
	
private:
	// �¼�ѭ����
	EventLoop* loop_;
	// �¼�������
	std::shared_ptr<CurlEvMgr> curl_evmgr_;
	// ����FD
	int wakeupFd_;
	// ����ͨ��
	std::shared_ptr<Channel> wakeupChannel_;
	// ��·���������
	CURLM* curlm_;
	// ��¼���ڱ���صļ򵥾�����ܸ���
	int runningHandles_;
	// ��¼��һ�μ�صļ򵥾�����ܸ���
	int prevRunningHandles_;
	// ��ʱ��ID
	TimerId timerid_;
};

} // end namespace curl


#endif // __CURL_CURLMANAGER_H__
