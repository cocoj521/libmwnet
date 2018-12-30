#ifndef __REQUEST_MANAGER_H__
#define __REQUEST_MANAGER_H__

#include "CurlRequest.h"
#include <mutex>
#include <map>
#include <list>

namespace curl
{
#define MAX_TOTAL_REQUESTING 	10000
#define MAX_HOST_REQUESTING 	100
#define MAX_WAIT_REQUEST 		50000

// 请求中
class HttpRequesting
{
public:
	static HttpRequesting& GetInstance()
	{
	    static HttpRequesting instance;
    	return instance;
	}
	~HttpRequesting(){};
public:
	void setMaxSize(int total_host, int single_host);
	CurlRequestPtr find(CURL* c);
	CurlRequestPtr get(CURL* c);
	void add(const CurlRequestPtr& request);
	void remove(const CurlRequestPtr& request);
	bool isFull();
private:
	HttpRequesting();
private:
	// 最大总并发数
	int max_total_requesting_;
	// 最大单host并发数
	int max_host_requesting_;
	// 正在请求中的请求
	std::mutex mutex_requesting_;
	std::map<uint64_t, CurlRequestPtr> requestings_;	
};

// 等待请求
class HttpWaitRequest
{
public:
	static HttpWaitRequest& GetInstance()
	{
		static HttpWaitRequest instance;
		return instance;
	}
	~HttpWaitRequest(){};
public:
	void setMaxSize(int max_wait);
	bool isFull();
	bool add(const CurlRequestPtr& request);
	CurlRequestPtr get();
private:
	HttpWaitRequest();
private:
	// 最大等待请求数
	int max_wait_request_;
	// 等待发送的请求
	std::mutex mutex_wait_request_;
	std::list<CurlRequestPtr> wait_requests_;
};

// 已回收的请求
class HttpRecycleRequest
{
public:
	static HttpRecycleRequest& GetInstance()
	{
		static HttpRecycleRequest instance;
		return instance;
	}
	~HttpRecycleRequest(){};
public:
	CurlRequestPtr get();
	void recycle(const CurlRequestPtr& request);
private:
	HttpRecycleRequest(){};
private:
	// 已回收的请求
	std::mutex mutex_recycle_request_;
	std::list<CurlRequestPtr> recycle_requests_;
};

} // end namespace curl


#endif // __REQUEST_MANAGER_H__
