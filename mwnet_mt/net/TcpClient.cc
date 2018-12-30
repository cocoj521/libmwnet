// Use of this source code is governed by a BSD-style license
// that can be found in the License file.

#include <mwnet_mt/net/TcpClient.h>

#include <mwnet_mt/base/Logging.h>
#include <mwnet_mt/net/Connector.h>
#include <mwnet_mt/net/EventLoop.h>
#include <mwnet_mt/net/SocketsOps.h>

#include <stdio.h>  // snprintf

using namespace mwnet_mt;
using namespace mwnet_mt::net;

// TcpClient::TcpClient(EventLoop* loop)
//   : loop_(loop)
// {
// }

// TcpClient::TcpClient(EventLoop* loop, const string& host, uint16_t port)
//   : loop_(CHECK_NOTNULL(loop)),
//     serverAddr_(host, port)
// {
// }

namespace mwnet_mt
{
	namespace net
	{
		namespace detail
		{

			void removeConnection(EventLoop* loop, const TcpConnectionPtr& conn)
			{
				loop->queueInLoop(std::bind(&TcpConnection::connectDestroyed, conn));
			}

			void removeConnector(const ConnectorPtr& connector)
			{
				//connector->
			}

		}
	}
}

TcpClient::TcpClient(EventLoop* loop,
	const InetAddress& serverAddr,
	const string& nameArg,
	size_t nDefRecvBuf,
	size_t nMaxRecvBuf,
	size_t nMaxSendQue)

	: loop_(CHECK_NOTNULL(loop)),
	connector_(new Connector(loop, serverAddr)),
	name_(nameArg),
	connectionCallback_(defaultConnectionCallback),
	messageCallback_(defaultMessageCallback),
	nextConnId_(1),
	serverAddr_(serverAddr),
	m_nDefRecvBuf(nDefRecvBuf),
	m_nMaxRecvBuf(nMaxRecvBuf),
	m_nMaxSendQue(nMaxSendQue)
{
	connector_->setNewConnectionCallback(std::bind(&TcpClient::newConnection, this, _1));
}

TcpClient::~TcpClient()
{
	TcpConnectionPtr conn;
	bool unique = false;
	{
		MutexLockGuard lock(mutex_);
		unique = connection_.unique();
		conn = connection_;
	}
	if (conn)
	{
		assert(loop_ == conn->getLoop());
		// FIXME: not 100% safe, if we are in different thread
		CloseCallback cb = std::bind(&detail::removeConnection, loop_, _1);
		loop_->runInLoop(std::bind(&TcpConnection::setCloseCallback, conn, cb));
		if (unique)
		{
			conn->shutdown();
			conn->forceClose();
		}
	}
	else
	{
		connector_->stop();
		// FIXME: HACK
		loop_->runAfter(1, std::bind(&detail::removeConnector, connector_));
	}
}

void TcpClient::setNewAddr(const InetAddress& serverAddr)
{
	connector_->setNewAddr(serverAddr);
}

void TcpClient::connect()
{
	connector_->start();
}

void TcpClient::disconnect()
{

	{
		MutexLockGuard lock(mutex_);
		if (connection_)
		{
			connection_->shutdown();
		}
	}
}

void TcpClient::stop()
{
	connector_->stop();
}

void TcpClient::quitconnect()
{

	{
		MutexLockGuard lock(mutex_);
		if (connection_)
		{
			connection_->shutdown();
			connection_->forceClose();
		}
	}
}

void TcpClient::enableRetry()
{
	connector_->enableRetry();
}

void TcpClient::newConnection(int sockfd)
{
	loop_->assertInLoopThread();
	if (-1 != sockfd)
	{
		InetAddress peerAddr(sockets::getPeerAddr(sockfd));
		/*
		char buf[32];
		snprintf(buf, sizeof buf, ":%s#%d", peerAddr.toIpPort().c_str(), nextConnId_);
		string connName = name_ + buf;
		*/
		InetAddress localAddr(sockets::getLocalAddr(sockfd));
		// FIXME poll with zero timeout to double confirm the new connection
		// FIXME use make_shared if necessary
		TcpConnectionPtr conn(new TcpConnection(loop_,
			sockfd,
			localAddr,
			peerAddr,
			m_nDefRecvBuf,
			m_nMaxRecvBuf,
			m_nMaxSendQue));

		++nextConnId_;

		conn->setConnectionCallback(connectionCallback_);
		conn->setMessageCallback(messageCallback_);
		conn->setWriteCompleteCallback(writeCompleteCallback_);

		conn->setCloseCallback(std::bind(&TcpClient::removeConnection, this, _1)); // FIXME: unsafe
		{
			MutexLockGuard lock(mutex_);
			connection_ = conn;
		}
		conn->connectEstablished();
	}
	else
	{
		TcpConnectionPtr conn(nullptr);
		connectionCallback_(conn);
	}
}

void TcpClient::removeConnection(const TcpConnectionPtr& conn)
{
	loop_->assertInLoopThread();
	assert(loop_ == conn->getLoop());

	{
		MutexLockGuard lock(mutex_);
		assert(connection_ == conn);
		connection_.reset();
	}

	loop_->queueInLoop(std::bind(&TcpConnection::connectDestroyed, conn));
}

