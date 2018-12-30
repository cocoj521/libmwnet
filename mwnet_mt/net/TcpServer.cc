// Use of this source code is governed by a BSD-style license
// that can be found in the License file.

#include <mwnet_mt/net/TcpServer.h>

#include <mwnet_mt/base/Logging.h>
#include <mwnet_mt/net/Acceptor.h>
#include <mwnet_mt/net/EventLoop.h>
#include <mwnet_mt/net/EventLoopThreadPool.h>
#include <mwnet_mt/net/SocketsOps.h>

#include <stdio.h>  // snprintf

using namespace mwnet_mt;
using namespace mwnet_mt::net;

TcpServer::TcpServer(EventLoop* loop, 
						const string& nameArg, 
						const InetAddress& listenAddr, 
						Option option, 
						size_t nDefRecvBuf, 
						size_t nMaxRecvBuf, 
						size_t nMaxSendQue)
  : m_loop(CHECK_NOTNULL(loop)),
    m_strIpPort(listenAddr.toIpPort()),
    m_strName(nameArg),
    //m_acceptor(new Acceptor(loop, listenAddr, option == kReusePort)),
    //m_threadPool(new EventLoopThreadPool(loop, nameArg)),
    m_connectionCallback(defaultConnectionCallback),
    m_messageCallback(defaultMessageCallback),
	m_nDefRecvBuf(nDefRecvBuf),
	m_nMaxRecvBuf(nMaxRecvBuf),
	m_nMaxSendQue(nMaxSendQue),
	stopLatch_(new CountDownLatch(1)),
	option_(option),
	listenAddr_(listenAddr)
{
	m_nNextSockId = 1;
}

TcpServer::~TcpServer()
{
	m_loop->assertInLoopThread();
	//LOG_INFO << "TcpServer::~TcpServer [" << name_ << "] destructing";

	// 关闭当前所有连接
	for (ConnectionMap::iterator it(m_mapConnections.begin()); it != m_mapConnections.end();)
	{
		TcpConnectionPtr conn(it->second);
		if (conn)
		{
			std::shared_ptr<CountDownLatch> latch(new CountDownLatch(1));
			conn->getLoop()->runInLoop(std::bind(&TcpConnection::connectDestroyedWithLatch, conn, latch));
			latch->wait();
		}
		m_mapConnections.erase(it++);
	}
}

void TcpServer::stop()
{
	m_loop->runInLoop(std::bind(&TcpServer::stopInLoop, this));
	stopLatch_->wait();
}

void TcpServer::stopInLoop()
{
	m_loop->assertInLoopThread();

	// 停止监听
	m_acceptor.reset();

	// 关闭当前所有连接
	for (ConnectionMap::iterator it(m_mapConnections.begin()); it != m_mapConnections.end();)
	{
		TcpConnectionPtr conn(it->second);
		if (conn)
		{
			std::shared_ptr<CountDownLatch> latch(new CountDownLatch(1));
			conn->getLoop()->runInLoop(std::bind(&TcpConnection::connectDestroyedWithLatch, conn, latch));
			latch->wait();
		}
		m_mapConnections.erase(it++);
	}

	// 退出线程池
	m_threadPool.reset();
	
	// 置为已关闭标记
	m_AtomicIntStarted.getAndSet(0);

	stopLatch_->countDown();
}

void TcpServer::setThreadNum(int numThreads)
{
	numThreads<=0?numThreads=1:1;
	m_nThreadNum = numThreads;
}

void TcpServer::start()
{
	stopLatch_->resetCount(1);

	if (m_AtomicIntStarted.getAndSet(1) == 0)
	{
		// 启动线程池
		m_threadPool.reset(new EventLoopThreadPool(m_loop, m_strName));

		m_threadPool->setThreadNum(m_nThreadNum); 

		m_threadPool->start(m_threadInitCallback);

		// 启动监听
		m_acceptor.reset(new Acceptor(m_loop, listenAddr_, (option_==kReusePort)));

		m_acceptor->setNewConnectionCallback(std::bind(&TcpServer::newConnection, this, _1, _2));

		m_loop->runInLoop(std::bind(&Acceptor::listen, get_pointer(m_acceptor)));
	}
}

void TcpServer::newConnection(int sockfd, const InetAddress& peerAddr)
{
	m_loop->assertInLoopThread();
	EventLoop* ioLoop = m_threadPool->getNextLoop();
	InetAddress localAddr(sockets::getLocalAddr(sockfd));
	//boost::uuids::random_generator gen;
	// FIXME poll with zero timeout to double confirm the new connection
	// FIXME use make_shared if necessary
	//int64_t nSockId = m_nNextSockId.incrementAndGet();
	TcpConnectionPtr conn(new TcpConnection(ioLoop,sockfd,localAddr,peerAddr,m_nDefRecvBuf,m_nMaxRecvBuf,m_nMaxSendQue));
	conn->setSockId(m_nNextSockId);
	m_mapConnections[m_nNextSockId++] = conn;
	//conn->setSockId(nSockId);
	//m_mapConnections[nSockId] = conn;
	conn->setConnectionCallback(m_connectionCallback);
	conn->setMessageCallback(m_messageCallback);
	conn->setWriteCompleteCallback(m_writeCompleteCallback);
	conn->setWriteErrCallback(m_writeErrCallback);
	conn->setCloseCallback(std::bind(&TcpServer::removeConnection, this, _1)); // FIXME: unsafe
	ioLoop->runInLoop(std::bind(&TcpConnection::connectEstablished, conn));
}

void TcpServer::removeConnection(const TcpConnectionPtr& conn)
{
	// FIXME: unsafe
	m_loop->runInLoop(std::bind(&TcpServer::removeConnectionInLoop, this, conn));
}

void TcpServer::removeConnectionInLoop(const TcpConnectionPtr& conn)
{
	m_loop->assertInLoopThread();
	size_t n = m_mapConnections.erase(conn->getSockId());
	(void)n;
	assert(n == 1);
	EventLoop* ioLoop = conn->getLoop();
	ioLoop->queueInLoop(std::bind(&TcpConnection::connectDestroyed, conn));
}

