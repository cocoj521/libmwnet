// Use of this source code is governed by a BSD-style license
// that can be found in the License file.


#include <mwnet_mt/net/Connector.h>

#include <mwnet_mt/base/Logging.h>
#include <mwnet_mt/net/Channel.h>
#include <mwnet_mt/net/EventLoop.h>
#include <mwnet_mt/net/SocketsOps.h>

#include <errno.h>

using namespace mwnet_mt;
using namespace mwnet_mt::net;

Connector::Connector(EventLoop* loop, const InetAddress& serverAddr)
  : loop_(loop),
    serverAddr_(serverAddr),
    connect_(false),
    state_(kDisconnected)
{
}

Connector::~Connector()
{
	assert(!channel_);
}

void Connector::start(int timeout_s)
{
	conn_timeout_ = timeout_s;
	connect_ = true;
	loop_->queueInLoop(std::bind(&Connector::startInLoop, shared_from_this())); 
}

void Connector::startInLoop()
{
	loop_->assertInLoopThread();
	assert(state_ == kDisconnected);
	if (connect_)
	{
		int nErrCode = 0;
		if (!connect(nErrCode))
		{
			setState(kDisconnected);
			newConnectionCallback_(-1, nErrCode, 0);
		}
	}
	else
	{
		LOG_DEBUG << "do not connect";
	}
}

void Connector::stop()
{
	connect_ = false;
	loop_->queueInLoop(std::bind(&Connector::stopInLoop, shared_from_this()));
}

void Connector::stopInLoop()
{
	loop_->assertInLoopThread();
	if (state_ == kConnecting)
	{
		LOG_INFO << "Connector::stopInLoop()";

		setState(kDisconnected);
		int sockfd = removeAndResetChannel();
		sockets::close(sockfd);

		newConnectionCallback_(-1, 0, 3);
	}
}

#include <errno.h>
#include <fcntl.h>
bool Connector::connect(int& nErrCode)
{
	bool bRet = false;

	int sockfd = sockets::createNonblockingOrDie(serverAddr_.family());
	if (sockfd > 0)
	{
		int ret = sockets::connect(sockfd, serverAddr_.getSockAddr());
		nErrCode = (ret == 0) ? 0 : errno;
		switch (nErrCode)
		{
			case 0:
			case EINPROGRESS:
			case EINTR:
			case EISCONN:
			{
				connecting(sockfd);
				bRet = true;
			}
				break;

			case EAGAIN:
			case EADDRINUSE:
			case EADDRNOTAVAIL:
			case ECONNREFUSED:
			case ENETUNREACH:
				sockets::close(sockfd);
				break;

			case EACCES:
			case EPERM:
			case EAFNOSUPPORT:
			case EALREADY:
			case EBADF:
			case EFAULT:
			case ENOTSOCK:
				sockets::close(sockfd);
				break;
			default:
				sockets::close(sockfd);
				break;
		}
	}
	return bRet;
}

void Connector::restart()
{
	loop_->assertInLoopThread();
	setState(kDisconnected);
	connect_ = true;
	startInLoop();
}

void Connector::connecting(int sockfd)
{
	setState(kConnecting);
	assert(!channel_);
	channel_.reset(new Channel(loop_, sockfd));
	channel_->tie(shared_from_this());
	channel_->setWriteCallback(std::bind(&Connector::handleWrite, this)); 
	channel_->setErrorCallback(std::bind(&Connector::handleError, this)); 
	channel_->doNotLogHup();

	// 启一个timer,处理连接超时
	if (conn_timeout_ > 0)
	{
		conn_timeout_timer_ = loop_->runAfter(conn_timeout_*1.0, std::bind(&Connector::handleConnTimeOut, shared_from_this()));
	}
 
	channel_->enableWriting();
}

int Connector::removeAndResetChannel()
{
	channel_->disableAll();
	channel_->remove();
	int sockfd = channel_->fd();
	// Can't reset channel_ here, because we are inside Channel::handleEvent
	loop_->queueInLoop(std::bind(&Connector::resetChannel, shared_from_this())); 
	return sockfd;
}

void Connector::resetChannel()
{
	channel_.reset();
}

void Connector::handleWrite()
{
	loop_->cancel(conn_timeout_timer_);

	if (state_ == kConnecting)
	{
		setState(kConnected);
		int sockfd = removeAndResetChannel();
		int err = sockets::getSocketError(sockfd);
		if (err)
		{
			setState(kDisconnected);
			sockets::close(sockfd);
			newConnectionCallback_(-1, err, 0);
		}
		else if (sockets::isSelfConnect(sockfd))
		{
			setState(kDisconnected);
			sockets::close(sockfd);
			newConnectionCallback_(-1, err, 2);
		}
		else
		{
			if (connect_)
			{
				setState(kConnected);
				newConnectionCallback_(sockfd, 0, 0);
			}
			else
			{
				setState(kDisconnected);
				sockets::close(sockfd);
				newConnectionCallback_(-1, err, 3);
			}
		}
	}
}

void Connector::handleError()
{
	LOG_ERROR << "Connector::handleError state=" << stateToString();

	loop_->cancel(conn_timeout_timer_);

	if (state_ == kConnecting)
	{
		setState(kDisconnected);
		int sockfd = removeAndResetChannel();
		int err = sockets::getSocketError(sockfd);
		sockets::close(sockfd);

		newConnectionCallback_(-1, err, 0);
	}
}

void Connector::handleConnTimeOut()
{
	loop_->queueInLoop(std::bind(&Connector::handleConnTimeOutInLoop, shared_from_this())); 
}

void Connector::handleConnTimeOutInLoop()
{
	LOG_ERROR << "Connector::handleConnTimeOut state=" << stateToString();

	loop_->assertInLoopThread();
	
	loop_->cancel(conn_timeout_timer_);

	if (state_ == kConnecting)
	{
		setState(kDisconnected);
		int sockfd = removeAndResetChannel();
		sockets::close(sockfd);

		newConnectionCallback_(-1, 0, 1);
	}
}

