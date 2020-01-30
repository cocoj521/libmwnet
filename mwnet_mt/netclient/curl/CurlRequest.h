#ifndef __CURL_REQUEST_H__
#define __CURL_REQUEST_H__

#include <curl/curl.h>
#include <functional>
#include <memory>
#include <string>
#include <boost/any.hpp>
#include <sys/time.h>
#include <mwnet_mt/net/EventLoop.h>
#include <mwnet_mt/net/TimerId.h>
#include <mwnet_mt/base/Timestamp.h>

extern "C"
{
	typedef void CURLM;
	typedef void CURL;
}
using namespace mwnet_mt;
using namespace mwnet_mt::net;

namespace curl
{

class CurlRequest;

typedef std::shared_ptr<CurlRequest> CurlRequestPtr;
typedef std::function<void(const CurlRequestPtr&, const char*, int)> DataCallback;
typedef std::function<void(const boost::any&, int, const char*)> DoneCallback;

class CurlManager;


/**
 * ��ʾHttp������������ԣ�ÿ����һ��http���󣬾ͻ�newһ��CurlRequest����
 * �ö���ͨ������ָ��������������ӳɹ������Map��ʧ�ܵĻ�������ʱʱ������Ӧ
 * ���ĳ�ʱ�ص�������delete����
 */
class CurlRequest : public std::enable_shared_from_this<CurlRequest>
{
public:
	/**
	 * [CurlRequest ���캯����GET����ʱ����]
	 * @param url  [URL��ַ]
	 * @param req_uuid  [Ψһ��ʶ]
	 */
	CurlRequest(const std::string& url, uint64_t req_uuid, bool bKeepAlive, int req_type, int http_ver);

	~CurlRequest();

	// ��ʼ���������
	void initCurlRequest(const std::string& url, uint64_t req_uuid, bool bKeepAlive, int req_type, int http_ver);
	
	/**
	 * ��������
	 */
	void request(CurlManager* cm, CURLM* multi, EventLoop* loop);

	// ǿ��ȡ������
	void forceCancel();
	// ǿ��ȡ������(�ڲ�����)
	void forceCancelInner();

	/**
	 * ��multi���Ƴ�
	 */
	void removeMultiHandle(CURLM* multi);
	
	//  ���ó�ʱʱ��
	// 	* @param conn_timeout   [���ӽ�ʱʱ�䣬Ĭ�ϣ�2s]
	//  * @param entire_timeout [����������̵ĳ�ʱʱ�䣬Ĭ�ϣ�5s]
	void setTimeOut(long conn_timeout=2, long entire_timeout=5);

	// KEEPIDLE : TCP keep-alive idle time wait
	// KEEPINTVL: interval time between keep-alive probes
	void setKeepAliveTime(long keep_idle=30, long keep_intvl=10);

	bool isKeepAlive() {return keep_alive_;}

	void setDnsCacheTimeOut(long cache_timeout=60);
	
	/**
	 * [setContext ����һ�������ģ�����������boost::any�����԰��κζ���Ž���]
	 * @param context [�κεĶ���ֵ����ָ��ȵ�]
	 */
	void setContext(const boost::any& context)
	{ context_ = context; }

	/**
	 * [getContext ȡ����������Ķ����ֵ����]
	 * @return [���������������]
	 */
	const boost::any& getContext() const
	{ return context_; }

	/**
	 * [setReqUUID �����������ڹ������Ψһ��ʶreq_uuid��ֵ]
	 * @param req_uuid [64λ������ֵ�������Ĺ���������Ҫ����Ψһ��]
	 */
	void setReqUUID(uint64_t req_uuid)
	{ req_uuid_ = req_uuid; }

	/**
	 * [getReqUUID ��ȡ�������ڹ������Ψһ��ʶreq_uuid��ֵ]
	 * @return [�������ڹ������Ψһ��ʶreq_uuid��ֵ]
	 */
	uint64_t getReqUUID() const
	{ return req_uuid_; }

	/**
	 * [setDataCallback ���ð���Ļص�����]
	 * @param cb [����Ļص�����]
	 */
	void setBodyCallback(const DataCallback& cb)
	{ bodyCb_ = cb; }

	/**
	 * [setDoneCallback ����������ɺ�Ļص�����]
	 * @param cb [��ɵĻص�����]
	 */
	void setDoneCallback(const DoneCallback& cb)
	{ doneCb_ = cb; }

	/**
	 * [setHeaderCallback ��������ͷ�Ļص�������ע������һ�лص�һ��]
	 * @param cb [��ͷ�ص�����]
	 */
	void setHeaderCallback(const DataCallback& cb)
	{ headerCb_ = cb; }

	/**
	 * [headerOnly ��ʶֻ�����ͷ��Ӧ���������ļ�ʱ�����������ļ�]
	 */
	void headerOnly();

	void setRange(const std::string& range);

	std::string getEffectiveUrl();
	std::string getRedirectUrl();

	/**
		���ػ�Ӧ�����壩
	*/
	std::string& getResponseHeader();

	/**
		���ػ�Ӧ�����壩
	*/
	std::string& getResponseBody();

	/*
		���ػ�Ӧ��ͷ�е�conetnt-type
	*/
	std::string getResponseContentType();
	
	/**
	 * [getResponseCode ��ȡ��Ӧ��״̬��]
	 * @return [�ɹ���200, ʧ�ܣ�4XX��5XX��]
	 */
	int getResponseCode();

	/**
	 * [done HTTP������ɺ�Ļص�����]
	 * @param errCode [������룺0 �ɹ� ���� ʧ��]
	 * @param errDesc [��������]
	 */
	void done(int errCode, const char* errDesc);

	/**
	 * [setHeaders ����HTTPͷ����ֵ]
	 * @param field  [�������]
	 * @param vale   [���ֵ]
	 */
	void setHeaders(const std::string& field, const std::string& value);

	/**
	 * [setBody ���ð���]
	 */
	void setBody(std::string& strBody);

	void setUserAgent(const std::string& strUserAgent);
	
	/**
	 * [getCurl ��ȡCURL��ָ��]
	 * @return [curl_ָ��]
	 */
	CURL* getCurl() { return curl_; }
	
private:

	// ��headers_���curl
	void setHeaders();
	// ���headers_�ڴ�
	void cleanHeaders();
	// ����curl���
	void cleanCurlHandles();

	/**
	 * [����ص����������ڲ�ʹ��]
	 * @param buffer [��Ҫ������ַ����׵�ַ]
	 * @param len    [��Ҫ������ַ�������]
	 */
	void responseBodyCallback(const char* buffer, int len);

	/**
	 * [��ͷ�ص����������ڲ�ʹ�ã�ÿ�лص�һ��]
	 * @param buffer [��Ҫ������ַ����׵�ַ]
	 * @param len    [��Ҫ������ַ�������]
	 */
	void responseHeaderCallback(const char* buffer, int len);

	/**
	 * [����ص�����, ��Ҫ��curl���ڲ�ע��]
	 * @param  buffer [��Ҫ������ַ����׵�ַ]
	 * @param  size   [��Ҫ������ַ�������]
	 * @param  nmemb  [��Ҫ������ַ���д��Ĵ���]
	 * @param  userp  [ע���Ǵ�����û��Լ��Ĳ���ָ��]
	 * @return        [д����ܴ�С��һ��Ϊ��size * nmemb]
	 */
	static size_t bodyDataCb(char* buffer, size_t size, size_t nmemb, void *userp);
	/**
	 * [��ͷ�ص�����,ÿ�лص�һ�Σ���Ҫ��curl���ڲ�ע��]
	 * @param  buffer [��Ҫ������ַ����׵�ַ]
	 * @param  size   [��Ҫ������ַ�������]
	 * @param  nmemb  [��Ҫ������ַ���д��Ĵ���]
	 * @param  userp  [ע���Ǵ�����û��Լ��Ĳ���ָ��]
	 * @return        [������ܴ�С��һ��Ϊ��size * nmemb]
	 */
	static size_t headerDataCb(char *buffer, size_t size, size_t nmemb, void *userp);

	// �ӹ�socket�ر��¼�
	static int hookCloseSocket(void *clientp,  int fd);

	// �ӹ�socket�����¼�
	static int hookOpenSocket(void *clientp, curlsocktype purpose, struct curl_sockaddr *address);
private:
	// �����Ωһid
	uint64_t req_uuid_;
	// curl��ʹ�õĵ�ַָ��
	CURL* curl_;
	// http����ͷָ��
	curl_slist* headers_;
	// �¼�ѭ����
	EventLoop* loop_;
	// curl������ָ��
	CurlManager* cm_;
	// ����ص�����
	DataCallback bodyCb_;
	// ��ͷ�ص�����
	DataCallback headerCb_;
	// ��ɻص�����
	DoneCallback doneCb_;
	// ����Ҫ����İ�ͷ
	std::string requestHeaderStr_;
	// ����Ҫ����İ���
	std::string requestBodyStr_;
	// ������Ӧ�İ���(��Ӧ���ݹ���ʱ���λص�dataCb_)
	std::string responseBodyStr_;
	// ������Ӧ�İ���ͷ+�壩
	std::string responseHeaderStr_;
	// ����������
	boost::any context_;
	// keepalive or close
	bool keep_alive_;
	// socket���
	int fd_;
	// POST/GET	
	int req_type_;
	std::string url_;
	int http_ver_;
public:
	uint32_t total_time_;
	uint32_t connect_time_;
	uint32_t namelookup_time_;
	uint64_t rsp_time_;
	uint64_t req_time_;
	uint64_t req_inque_time_;
	TimerId timerid_;
	long entire_timeout_;
};

} // end namespace curl

#endif // __CURL_REQUEST_H__
