#include "RequestManager.h"
#include <mwnet_mt/base/Logging.h>

using namespace curl;

HttpRequesting::HttpRequesting()
	:
	max_total_requesting_(MAX_TOTAL_REQUESTING),
	max_host_requesting_(MAX_HOST_REQUESTING)
{

}

void HttpRequesting::setMaxSize(int total_host, int single_host)
{
	//std::lock_guard<std::mutex> lock(mutex_requesting_);
	max_total_requesting_ = total_host;
	max_host_requesting_  = single_host;
}

bool HttpRequesting::isFull()
{
	//std::lock_guard<std::mutex> lock(mutex_requesting_);
	return (static_cast<int>(requestings_.size()) > max_total_requesting_);
}

void HttpRequesting::add(const CurlRequestPtr& request)
{
	std::lock_guard<std::mutex> lock(mutex_requesting_);
	requestings_.insert(std::pair<uint64_t, CurlRequestPtr>(request->getReqUUID(), request));
}

void HttpRequesting::remove(const CurlRequestPtr& request)
{
	std::lock_guard<std::mutex> lock(mutex_requesting_);
	requestings_.erase(request->getReqUUID());
}

CurlRequestPtr HttpRequesting::find(CURL* c)
{
	CurlRequestPtr p = nullptr;
	
	void* userp = NULL;
	curl_easy_getinfo(c, CURLINFO_PRIVATE, &userp);
	uint64_t req_uuid = reinterpret_cast<uint64_t>(userp);
	std::lock_guard<std::mutex> lock(mutex_requesting_);
	std::map<uint64_t, CurlRequestPtr>::iterator it = requestings_.find(req_uuid);
	if (it != requestings_.end())
	{
		p = it->second;
	}
	
	return p;
}

CurlRequestPtr HttpRequesting::find(uint64_t req_uuid)
{
	CurlRequestPtr p = nullptr;

	std::lock_guard<std::mutex> lock(mutex_requesting_);
	std::map<uint64_t, CurlRequestPtr>::iterator it = requestings_.find(req_uuid);
	if (it != requestings_.end())
	{
		p = it->second;
	}
	
	return p;
}

CurlRequestPtr HttpRequesting::get(CURL* c)
{
	CurlRequestPtr p = nullptr;
	
	void* userp = NULL;
	curl_easy_getinfo(c, CURLINFO_PRIVATE, &userp);
	uint64_t req_uuid = reinterpret_cast<uint64_t>(userp);
	std::lock_guard<std::mutex> lock(mutex_requesting_);
	std::map<uint64_t, CurlRequestPtr>::iterator it = requestings_.find(req_uuid);
	if (it != requestings_.end())
	{
		p = it->second;
		requestings_.erase(it);
	}
	
	return p;
}

// 清空前会回调所有未返回的请求
void HttpRequesting::clear()
{
	std::lock_guard<std::mutex> lock(mutex_requesting_);
	std::map<uint64_t, CurlRequestPtr>::iterator it = requestings_.begin();
	for (; it != requestings_.end(); )
	{
		it->second->forceCancelInner();
		requestings_.erase(it++);
	}
}

///////////////////////////////////////////////////////////////////
HttpWaitRequest::HttpWaitRequest()
	:
	max_wait_request_(MAX_WAIT_REQUEST),
	list_size_(0)
{
}

void HttpWaitRequest::setMaxSize(int max_wait)
{
	//std::lock_guard<std::mutex> lock(mutex_wait_request_);
	max_wait_request_ = max_wait;
}

int  HttpWaitRequest::size()
{
	//std::lock_guard<std::mutex> lock(mutex_wait_request_);
	return list_size_;
}

bool HttpWaitRequest::isFull()
{
	std::lock_guard<std::mutex> lock(mutex_wait_request_);
	return (list_size_ > max_wait_request_);
}

bool HttpWaitRequest::add(const CurlRequestPtr& request)
{
	bool bRet = false;
	
	std::lock_guard<std::mutex> lock(mutex_wait_request_);
	if (list_size_ < max_wait_request_)
	{
		wait_requests_.push_back(request);
		++list_size_;
		bRet = true;
	}

	return bRet;
}

CurlRequestPtr HttpWaitRequest::get()
{
	CurlRequestPtr p = nullptr;
	
	std::lock_guard<std::mutex> lock(mutex_wait_request_);
	if (!wait_requests_.empty())
	{
		p = wait_requests_.front();
		wait_requests_.pop_front();
		--list_size_;
	}
	
	return p;
}

// 清空前会回调所有未返回的请求
void HttpWaitRequest::clear()
{	
	std::lock_guard<std::mutex> lock(mutex_wait_request_);
	std::list<CurlRequestPtr>::iterator it = wait_requests_.begin();
	for (; it != wait_requests_.end(); )
	{
		(*it)->req_time_ = Timestamp::GetCurrentTimeUs();
		(*it)->done(10055, "Force Cancel Request");
		wait_requests_.erase(it++);
		--list_size_;
	}
}

/////////////////////////////////////////////////////////////////
CurlRequestPtr HttpRecycleRequest::get()
{
	CurlRequestPtr p = nullptr;
	
	std::lock_guard<std::mutex> lock(mutex_recycle_request_);

	if (!recycle_requests_.empty())
	{
		p = recycle_requests_.front();
		recycle_requests_.pop_front();
		--list_size_;
	}

	return p;
}

void HttpRecycleRequest::setMaxSize(int max_recycle_cnt)
{
	//std::lock_guard<std::mutex> lock(mutex_recycle_request_);
	max_recycle_cnt_ = max_recycle_cnt;
}

void HttpRecycleRequest::recycle(const CurlRequestPtr& request)
{
	std::lock_guard<std::mutex> lock(mutex_recycle_request_);
	//request->delContext();
	// 保持recycle队列的大小不超出设定的值
	if (list_size_ >= max_recycle_cnt_)
	{
		recycle_requests_.pop_front();
		--list_size_;
	}
	recycle_requests_.push_back(request);
	++list_size_;
}

void HttpRecycleRequest::clear()
{
	std::lock_guard<std::mutex> lock(mutex_recycle_request_);
	recycle_requests_.clear();
	list_size_ = 0;
}