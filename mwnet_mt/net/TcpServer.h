// Use of this source code is governed by a BSD-style license
// that can be found in the License file.

// This is a public header file, it must only include public header files.

#ifndef MWNET_MT_NET_TCPSERVER_H
#define MWNET_MT_NET_TCPSERVER_H

#include <mwnet_mt/base/Atomic.h>
#include <mwnet_mt/base/Types.h>
#include <mwnet_mt/base/CountDownLatch.h>
#include <mwnet_mt/net/TcpConnection.h>

#include <map>

namespace mwnet_mt
{
namespace net
{

class Acceptor;
class EventLoop;
class EventLoopThreadPool;

///
/// TCP server, supports single-threaded and thread-pool models.
///
/// This is an interface class, so don't expose too much details.
class TcpServer : noncopyable,
					public std::enable_shared_from_this<TcpServer>

{
static const size_t kInitialRecvBuf = 4*1024;
static const size_t kInitialSendQue = 512;

 public:
  typedef std::function<void(EventLoop*)> ThreadInitCallback;
  enum Option
  {
    kNoReusePort,
    kReusePort,
  };
  TcpServer(EventLoop* loop, 
			  	const string& nameArg, 
			  	const InetAddress& listenAddr, 
			  	Option option = kReusePort, 
				size_t nDefRecvBuf = kInitialRecvBuf, 
				size_t nMaxRecvBuf = kInitialRecvBuf, 
				size_t nMaxSendQue = kInitialSendQue);

  ~TcpServer();  // force out-line dtor, for scoped_ptr members.

  const string& ipPort() const { return m_strIpPort; }
  
  EventLoop* getLoop() const { return m_loop; }

  /// Set the number of threads for handling input.
  ///
  /// Always accepts new connection in loop's thread.
  /// Must be called before @c start
  /// @param numThreads
  /// - 0 means all I/O in loop's thread, no thread will created.
  ///   this is the default value.
  /// - 1 means all I/O in another thread.
  /// - N means a thread pool with N threads, new connections
  ///   are assigned on a round-robin basis.
  void setThreadNum(int numThreads);
  void setThreadInitCallback(const ThreadInitCallback& cb)
  		{ m_threadInitCallback = cb; }
  /// valid after calling start()
  std::shared_ptr<EventLoopThreadPool> threadPool()
  		{ return m_threadPool; }

  /// Starts the server if it's not listenning.
  ///
  /// It's harmless to call it multiple times.
  /// Thread safe.
  void start();
  void stop();
  void stopInLoop();
  
  /// Set connection callback.
  /// Not thread safe.
  void setConnectionCallback(const ConnectionCallback& cb)
  		{ m_connectionCallback = cb; }

  /// Set message callback.
  /// Not thread safe.
  void setMessageCallback(const MessageCallback& cb)
  		{ m_messageCallback = cb; }

  /// Set write complete callback.
  /// Not thread safe.
  void setWriteCompleteCallback(const WriteCompleteCallback& cb)
  {
	  m_writeCompleteCallback = cb;
  }
  void setWriteErrCallback(const WriteErrCallback& cb)
  {
	  m_writeErrCallback = cb;
  }
 private:
  /// Not thread safe, but in loop
  void newConnection(int sockfd, const InetAddress& peerAddr);
  /// Thread safe.
  void removeConnection(const TcpConnectionPtr& conn);
  /// Not thread safe, but in loop
  void removeConnectionInLoop(const TcpConnectionPtr& conn);

  typedef std::map<int64_t, TcpConnectionPtr> ConnectionMap;

  EventLoop* m_loop;  // the acceptor loop
  const string m_strIpPort;
  const string m_strName;
  std::unique_ptr<Acceptor> m_acceptor; // avoid revealing Acceptor
  std::shared_ptr<EventLoopThreadPool> m_threadPool;
  ConnectionCallback m_connectionCallback;
  MessageCallback m_messageCallback;
  WriteCompleteCallback m_writeCompleteCallback;
  WriteErrCallback m_writeErrCallback;
  ThreadInitCallback m_threadInitCallback;
  AtomicInt32 m_AtomicIntStarted;
  // always in loop thread
  //AtomicInt64 m_nNextSockId;
  int64_t m_nNextSockId;
  int m_nThreadNum;
  ConnectionMap m_mapConnections;
  size_t m_nDefRecvBuf;
  size_t m_nMaxRecvBuf;
  size_t m_nMaxSendQue;
  std::unique_ptr<CountDownLatch> stopLatch_;
  Option option_;
  InetAddress listenAddr_;
};

}
}

#endif  // MWNET_MT_NET_TCPSERVER_H
