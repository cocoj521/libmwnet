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
	connectionCallback_(NULL),
	messageCallback_(NULL),
	serverAddr_(serverAddr),
	m_nDefRecvBuf(nDefRecvBuf),
	m_nMaxRecvBuf(nMaxRecvBuf),
	m_nMaxSendQue(nMaxSendQue)
{
	connector_->setNewConnectionCallback(std::bind(&TcpClient::newConnection, this, _1, _2, _3));
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
		//loop_->runAfter(1, std::bind(&detail::removeConnector, connector_));
		loop_->queueInLoop(std::bind(&detail::removeConnector, connector_));
	}
	//LOG_INFO << "TcpClient::~TcpClient()";
}

void TcpClient::setNewAddr(const InetAddress& serverAddr)
{
	connector_->setNewAddr(serverAddr);
}

void TcpClient::connect()
{
	connector_->start();
}

void TcpClient::connect(int conn_timeout)
{
	connector_->start(conn_timeout);
}

void TcpClient::disconnect()
{
	MutexLockGuard lock(mutex_);
	if (connection_)
	{
		connection_->shutdown();
		connection_->forceClose();
	}
}

void TcpClient::stop()
{
	connector_->stop();
}

void TcpClient::enableRetry()
{
	connector_->enableRetry();
}

void TcpClient::onNewConnection(const TcpConnectionPtr& conn)
{
	if (conn->connected())
	{
		connectionCallback_(conn, "Connection established");
	}
	else
	{
		connectionCallback_(conn, "Connection closed");
	}
}

void TcpClient::newConnection(int sockfd, int sockErr, int otherErr)
{
	loop_->assertInLoopThread();
	if (sockfd > 0)
	{
		InetAddress peerAddr(sockets::getPeerAddr(sockfd));
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

		conn->tie(shared_from_this());
		conn->setConnectionCallback(std::bind(&TcpClient::onNewConnection, this, _1));
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
		std::string strErrDesc = "Other error";
		if (sockErr != 0)
		{
			strErrDesc = strerror_tl(sockErr);
		}
		else if (1 == otherErr)
		{
			strErrDesc = "Connect timeout";
		}
		else if (2 == otherErr)
		{
			strErrDesc = "Tcp self connection occurs";
		}
		else if (3 == otherErr)
		{
			strErrDesc = "Connect aborted manually";
		}
		else
		{
			strErrDesc = "Other error";
		}
		connectionCallback_(nullptr, strErrDesc);
	}
}

void TcpClient::removeConnection(const TcpConnectionPtr& conn)
{
	loop_->assertInLoopThread();
	assert(loop_ == conn->getLoop());

	if (connection_)
	{
		MutexLockGuard lock(mutex_);
		assert(connection_ == conn);
		connection_.reset();
	}

	loop_->queueInLoop(std::bind(&TcpConnection::connectDestroyed, conn));
}

// connection_析构了以后TcpClient才会析构,因为每个Connection建立后都tie了TcpClient的指针