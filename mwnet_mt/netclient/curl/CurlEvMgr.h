#ifndef __CURL_EVMGR_H__
#define __CURL_EVMGR_H__

#include "CurlEvMgr.h"
#include <mwnet_mt/net/Channel.h>
#include <mwnet_mt/net/EventLoop.h>

using namespace mwnet_mt;
using namespace mwnet_mt::net;
namespace curl
{
class CurlEvMgr :public std::enable_shared_from_this<CurlEvMgr> 
{
public:
	typedef std::function<void()> EventCallback;
	typedef std::function<void(Timestamp)> ReadEventCallback;
public:
	CurlEvMgr(EventLoop* loop);
	~CurlEvMgr();
public:
	void* getNewEv(int fd);
	void  delEv(void* p);
	// 生成新channel,并将fd与channel关联
	void* addEvLoop(void* p, int fd, int what, const ReadEventCallback& read_cb, const EventCallback& write_cb);
	void* delEvLoop(void* p, int fd, int what);
	void  optEvLoop(void* p, int fd, int what);
private:
	void closeFd(int fd);
private:
	// 事件循环器
	EventLoop* loop_;
};

} // end namespace curl


#endif // __CURL_EVMGR_H__
