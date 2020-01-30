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
 * 表示Http请求的连接属性，每调用一次http请求，就会new一个CurlRequest对象，
 * 该对象通过智能指针来管理，如果连接成功则放入Map，失败的话，到超时时间后会响应
 * 它的超时回调函数，delete对象。
 */
class CurlRequest : public std::enable_shared_from_this<CurlRequest>
{
public:
	/**
	 * [CurlRequest 构造函数，GET请求时调用]
	 * @param url  [URL地址]
	 * @param req_uuid  [唯一标识]
	 */
	CurlRequest(const std::string& url, uint64_t req_uuid, bool bKeepAlive, int req_type, int http_ver);

	~CurlRequest();

	// 初始化请求参数
	void initCurlRequest(const std::string& url, uint64_t req_uuid, bool bKeepAlive, int req_type, int http_ver);
	
	/**
	 * 发起请求
	 */
	void request(CurlManager* cm, CURLM* multi, EventLoop* loop);

	// 强制取消请求
	void forceCancel();
	// 强制取消请求(内部调用)
	void forceCancelInner();

	/**
	 * 从multi中移除
	 */
	void removeMultiHandle(CURLM* multi);
	
	//  设置超时时间
	// 	* @param conn_timeout   [连接建时时间，默认：2s]
	//  * @param entire_timeout [整个请求过程的超时时间，默认：5s]
	void setTimeOut(long conn_timeout=2, long entire_timeout=5);

	// KEEPIDLE : TCP keep-alive idle time wait
	// KEEPINTVL: interval time between keep-alive probes
	void setKeepAliveTime(long keep_idle=30, long keep_intvl=10);

	bool isKeepAlive() {return keep_alive_;}

	void setDnsCacheTimeOut(long cache_timeout=60);
	
	/**
	 * [setContext 设置一个上下文，参数类型是boost::any，可以把任何对象放进来]
	 * @param context [任何的对象：值对象，指针等等]
	 */
	void setContext(const boost::any& context)
	{ context_ = context; }

	/**
	 * [getContext 取放入的上下文对象的值引用]
	 * @return [放入的上下文引用]
	 */
	const boost::any& getContext() const
	{ return context_; }

	/**
	 * [setReqUUID 设置它在所在管理类的唯一标识req_uuid的值]
	 * @param req_uuid [64位整数的值，在它的管理类中需要保持唯一性]
	 */
	void setReqUUID(uint64_t req_uuid)
	{ req_uuid_ = req_uuid; }

	/**
	 * [getReqUUID 获取它在所在管理类的唯一标识req_uuid的值]
	 * @return [它在所在管理类的唯一标识req_uuid的值]
	 */
	uint64_t getReqUUID() const
	{ return req_uuid_; }

	/**
	 * [setDataCallback 设置包体的回调函数]
	 * @param cb [包体的回调函数]
	 */
	void setBodyCallback(const DataCallback& cb)
	{ bodyCb_ = cb; }

	/**
	 * [setDoneCallback 设置请求完成后的回调函数]
	 * @param cb [完成的回调函数]
	 */
	void setDoneCallback(const DoneCallback& cb)
	{ doneCb_ = cb; }

	/**
	 * [setHeaderCallback 设置请求头的回调函数，注意它是一行回调一次]
	 * @param cb [包头回调函数]
	 */
	void setHeaderCallback(const DataCallback& cb)
	{ headerCb_ = cb; }

	/**
	 * [headerOnly 标识只请求包头，应用于下载文件时多链接下载文件]
	 */
	void headerOnly();

	void setRange(const std::string& range);

	std::string getEffectiveUrl();
	std::string getRedirectUrl();

	/**
		返回回应包（体）
	*/
	std::string& getResponseHeader();

	/**
		返回回应包（体）
	*/
	std::string& getResponseBody();

	/*
		返回回应包头中的conetnt-type
	*/
	std::string getResponseContentType();
	
	/**
	 * [getResponseCode 获取响应的状态码]
	 * @return [成功：200, 失败：4XX，5XX等]
	 */
	int getResponseCode();

	/**
	 * [done HTTP请求完成后的回调函数]
	 * @param errCode [错误代码：0 成功 其他 失败]
	 * @param errDesc [错误描述]
	 */
	void done(int errCode, const char* errDesc);

	/**
	 * [setHeaders 设置HTTP头属性值]
	 * @param field  [域的名称]
	 * @param vale   [域的值]
	 */
	void setHeaders(const std::string& field, const std::string& value);

	/**
	 * [setBody 设置包体]
	 */
	void setBody(std::string& strBody);

	void setUserAgent(const std::string& strUserAgent);
	
	/**
	 * [getCurl 获取CURL的指针]
	 * @return [curl_指针]
	 */
	CURL* getCurl() { return curl_; }
	
private:

	// 将headers_设进curl
	void setHeaders();
	// 清除headers_内存
	void cleanHeaders();
	// 清理curl句柄
	void cleanCurlHandles();

	/**
	 * [包体回调函数，供内部使用]
	 * @param buffer [需要处理的字符串首地址]
	 * @param len    [需要处理的字符串长度]
	 */
	void responseBodyCallback(const char* buffer, int len);

	/**
	 * [包头回调函数，供内部使用，每行回调一次]
	 * @param buffer [需要处理的字符串首地址]
	 * @param len    [需要处理的字符串长度]
	 */
	void responseHeaderCallback(const char* buffer, int len);

	/**
	 * [包体回调函数, 需要向curl库内部注册]
	 * @param  buffer [需要处理的字符串首地址]
	 * @param  size   [需要处理的字符串长度]
	 * @param  nmemb  [需要处理的字符串写入的次数]
	 * @param  userp  [注册是传入的用户自己的参数指针]
	 * @return        [写入的总大小，一般为：size * nmemb]
	 */
	static size_t bodyDataCb(char* buffer, size_t size, size_t nmemb, void *userp);
	/**
	 * [包头回调函数,每行回调一次，需要向curl库内部注册]
	 * @param  buffer [需要处理的字符串首地址]
	 * @param  size   [需要处理的字符串长度]
	 * @param  nmemb  [需要处理的字符串写入的次数]
	 * @param  userp  [注册是传入的用户自己的参数指针]
	 * @return        [处理的总大小，一般为：size * nmemb]
	 */
	static size_t headerDataCb(char *buffer, size_t size, size_t nmemb, void *userp);

	// 接管socket关闭事件
	static int hookCloseSocket(void *clientp,  int fd);

	// 接管socket创建事件
	static int hookOpenSocket(void *clientp, curlsocktype purpose, struct curl_sockaddr *address);
private:
	// 请求的惟一id
	uint64_t req_uuid_;
	// curl库使用的地址指针
	CURL* curl_;
	// http请求头指针
	curl_slist* headers_;
	// 事件循环器
	EventLoop* loop_;
	// curl管理类指针
	CurlManager* cm_;
	// 包体回调函数
	DataCallback bodyCb_;
	// 包头回调函数
	DataCallback headerCb_;
	// 完成回调函数
	DoneCallback doneCb_;
	// 保存要请求的包头
	std::string requestHeaderStr_;
	// 保存要请求的包体
	std::string requestBodyStr_;
	// 保存响应的包体(响应数据过长时会多次回调dataCb_)
	std::string responseBodyStr_;
	// 保存响应的包（头+体）
	std::string responseHeaderStr_;
	// 上下文数据
	boost::any context_;
	// keepalive or close
	bool keep_alive_;
	// socket句柄
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
