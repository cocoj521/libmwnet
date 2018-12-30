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
	max_total_requesting_ = total_host;
	max_host_requesting_  = single_host;
}

bool HttpRequesting::isFull()
{
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

///////////////////////////////////////////////////////////////////
HttpWaitRequest::HttpWaitRequest()
	:
	max_wait_request_(MAX_WAIT_REQUEST)
{
}

void HttpWaitRequest::setMaxSize(int max_wait)
{
	max_wait_request_ = max_wait;
}

bool HttpWaitRequest::isFull()
{
	return (static_cast<int>(wait_requests_.size()) > max_wait_request_);
}

bool HttpWaitRequest::add(const CurlRequestPtr& request)
{
	bool bRet = false;
	
	std::lock_guard<std::mutex> lock(mutex_wait_request_);
	if (static_cast<int>(wait_requests_.size()) < max_wait_request_)
	{
		wait_requests_.push_back(request);
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
	}
	
	return p;
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
	}

	return p;
}

void HttpRecycleRequest::recycle(const CurlRequestPtr& request)
{
	std::lock_guard<std::mutex> lock(mutex_recycle_request_);
	recycle_requests_.push_back(request);
}
