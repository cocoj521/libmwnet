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
	CurlRequestPtr find(uint64_t req_uuid);
	CurlRequestPtr get(CURL* c);
	void add(const CurlRequestPtr& request);
	void remove(const CurlRequestPtr& request);
	bool isFull();
	// 清空前会回调所有未返回的请求
	void clear();
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
	int  size();
	// 清空前会回调所有未返回的请求
	void clear();
	CurlRequestPtr get();
private:
	HttpWaitRequest();
private:
	// 最大等待请求数
	int max_wait_request_;
	// 等待发送的请求
	std::mutex mutex_wait_request_;
	std::list<CurlRequestPtr> wait_requests_;
	int list_size_;
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
	void setMaxSize(int max_recycle_cnt);
	void recycle(const CurlRequestPtr& request);
	void clear();
private:
	HttpRecycleRequest()
	{
		list_size_=0;
		max_recycle_cnt_=MAX_TOTAL_REQUESTING;
	};
private:
	int max_recycle_cnt_;
	// 已回收的请求
	std::mutex mutex_recycle_request_;
	std::list<CurlRequestPtr> recycle_requests_;
	int list_size_;
};

} // end namespace curl


#endif // __REQUEST_MANAGER_H__
