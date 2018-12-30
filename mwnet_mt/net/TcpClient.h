// Use of this source code is governed by a BSD-style license
// that can be found in the License file.
//
// This is a public header file, it must only include public header files.

#ifndef MWNET_MT_NET_TCPCLIENT_H
#define MWNET_MT_NET_TCPCLIENT_H

#include <mwnet_mt/base/Mutex.h>
#include <mwnet_mt/net/TcpConnection.h>

namespace mwnet_mt
{
namespace net
{
static const size_t kInitialRecvBuf = 4*1024;
static const size_t kInitialSendQue = 512;

class Connector;
typedef std::shared_ptr<Connector> ConnectorPtr;

class TcpClient : noncopyable
{
 public:
  // TcpClient(EventLoop* loop);
  // TcpClient(EventLoop* loop, const string& host, uint16_t port);
  TcpClient(EventLoop* loop,
				const InetAddress& serverAddr,
				const string& nameArg, 
				size_t nDefRecvBuf = kInitialRecvBuf, 
				size_t nMaxRecvBuf = kInitialRecvBuf, 
				size_t nMaxSendQue = kInitialSendQue);

  ~TcpClient();  // force out-line dtor, for std::unique_ptr members.

  void setNewAddr(const InetAddress& serverAddr);
  const InetAddress& getInetAddress() const {return serverAddr_;}
  void connect();
  void disconnect();
  void stop();
  void quitconnect();

  TcpConnectionPtr connection() const
  {
    MutexLockGuard lock(mutex_);
    return connection_;
  }

  EventLoop* getLoop() const { return loop_; }
  void enableRetry();
  
  const string& name() const
  { return name_; }

  /// Set connection callback.
  /// Not thread safe.
  void setConnectionCallback(const ConnectionCallback& cb)
  { connectionCallback_ = cb; }

  /// Set message callback.
  /// Not thread safe.
  void setMessageCallback(const MessageCallback& cb)
  { messageCallback_ = cb; }

  /// Set write complete callback.
  /// Not thread safe.
  void setWriteCompleteCallback(const WriteCompleteCallback& cb)
  { writeCompleteCallback_ = cb; }

#ifdef __GXX_EXPERIMENTAL_CXX0X__DONOT_USE
  void setConnectionCallback(ConnectionCallback&& cb)
  { connectionCallback_ = std::move(cb); }
  void setMessageCallback(MessageCallback&& cb)
  { messageCallback_ = std::move(cb); }
  void setWriteCompleteCallback(WriteCompleteCallback&& cb)
  { writeCompleteCallback_ = std::move(cb); }
#endif

 private:
  /// Not thread safe, but in loop
  void newConnection(int sockfd);
  /// Not thread safe, but in loop
  void removeConnection(const TcpConnectionPtr& conn);

  EventLoop* loop_;
  ConnectorPtr connector_; // avoid revealing Connector
  const string name_;
  ConnectionCallback connectionCallback_;
  MessageCallback messageCallback_;
  WriteCompleteCallback writeCompleteCallback_;
  // always in loop thread
  int nextConnId_;
  mutable MutexLock mutex_;
  TcpConnectionPtr connection_; // @GuardedBy mutex_
  InetAddress serverAddr_;
  size_t m_nDefRecvBuf;
  size_t m_nMaxRecvBuf;
  size_t m_nMaxSendQue;
};

}
}

#endif  // MWNET_MT_NET_TCPCLIENT_H
