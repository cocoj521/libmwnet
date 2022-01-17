#include "CurlEvMgr.h"
#include <mwnet_mt/base/Logging.h>
#include <curl/curl.h>
#include <mutex>

using namespace curl;

CurlEvMgr::CurlEvMgr(EventLoop* loop)
	: loop_(loop)
{
}

CurlEvMgr::~CurlEvMgr()
{
}

void* CurlEvMgr::getNewEv(int fd)
{
	Channel* ch = new Channel(loop_, fd);
	return static_cast<void*>(ch);
}

void  CurlEvMgr::delEv(void* p)
{
	Channel* ch = static_cast<Channel*>(p);
	if (ch)
	{
		delete ch;
	}
}

// ������channel,����fd��channel����
void* CurlEvMgr::addEvLoop(void* p, int fd, int what, 
	const ReadEventCallback& read_cb, const EventCallback& write_cb,
	const EventCallback& close_cb, const EventCallback& err_cb)
{
	Channel* ch = static_cast<Channel*>(p);
	
	LOG_DEBUG << "CurlEvMgr::addEvLoop:" << fd << "-" << ch << "-" << what;
	
	if (ch)
	{
		ch->tie(shared_from_this());
		
		// ���ö�/д/�ر�/�쳣�ص�����
		ch->setReadCallback(read_cb);
		ch->setWriteCallback(write_cb);
		//ch->setCloseCallback(close_cb);
		//ch->setErrorCallback(err_cb);

		// ��ʶͨ���ɶ�������ӦPOLL_IN�¼���
		ch->enableReading();
		
		return static_cast<void*>(ch);
	}

	return NULL;
}

void* CurlEvMgr::delEvLoop(void* p, int fd, int what)
{
	Channel* ch = static_cast<Channel*>(p);
	
	LOG_DEBUG << "CurlEvMgr::delEvLoop:" << fd << "-" << ch << "-" << what;
	
	if (ch)
	{
		// �ر�ͨ��
		ch->disableAll();
		ch->remove();

		// �ر�socket
		closeFd(ch->fd());

		// �ͷ�ͨ��ָ��
		delete ch;
	}
	
	return NULL;
}

// �ر�socket
void CurlEvMgr::closeFd(int fd)
{
	LOG_DEBUG << "CurlEvMgr::closeFd:" << fd;

	if (fd > 0)
	{
		::shutdown(fd, SHUT_RDWR);
		::close(fd);
	}
}

void CurlEvMgr::optEvLoop(void* p, int fd, int what)
{
	Channel* ch = static_cast<Channel*>(p);
	
	LOG_DEBUG << "CurlEvMgr::optEvLoop:" << fd << "-" << ch << "-" << what;
	
	if (ch)
	{
		if (what & CURL_POLL_OUT)
		{
			// ͨ����д
			ch->enableWriting();
		}
		else
		{
			ch->disableWriting();
		}
	}
}

