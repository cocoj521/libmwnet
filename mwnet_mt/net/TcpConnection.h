// Use of this source code is governed by a BSD-style license
// that can be found in the License file.
//
// This is a public header file, it must only include public header files.

#ifndef MWNET_MT_NET_TCPCONNECTION_H
#define MWNET_MT_NET_TCPCONNECTION_H

#include <mwnet_mt/base/StringPiece.h>
#include <mwnet_mt/base/Types.h>
#include <mwnet_mt/base/CountDownLatch.h>
#include <mwnet_mt/net/TimerId.h>
#include <mwnet_mt/net/Callbacks.h>
#include <mwnet_mt/net/Buffer.h>
#include <mwnet_mt/net/InetAddress.h>
#include <list>
#include <memory>
#include <atomic>

#include <boost/any.hpp>

// struct tcp_info is in <netinet/tcp.h>
struct tcp_info;

namespace mwnet_mt
{
namespace net
{

class Channel;
class EventLoop;
class Socket;

struct OUTPUT_STRUCT
{
	boost::any params;
	TimerId timerid_;
	Buffer outputBuffer_;

	OUTPUT_STRUCT(size_t nBufSize):
		outputBuffer_(nBufSize)
	{

	}
};
typedef std::shared_ptr<OUTPUT_STRUCT> OUTBUFFER_PTR;
typedef std::list<OUTBUFFER_PTR>::iterator OUTBUFFER_LIST_IT;

///
/// TCP connection, for both client and server usage.
///
/// This is an interface class, so don't expose too much details.
class TcpConnection : noncopyable,
                      public std::enable_shared_from_this<TcpConnection>
{
	static const size_t kInitialRecvBuf = 4*1024;
	static const size_t kInitialSendQue = 512;

 public:
  /// Constructs a TcpConnection with a connected sockfd
  ///
  /// User should not create this object.
  TcpConnection(EventLoop* loop, 
					 int sockfd, 
					 const InetAddress& localAddr, 
					 const InetAddress& peerAddr, 
					 size_t nDefRecvBuf = kInitialRecvBuf, 
					 size_t nMaxRecvBuf = kInitialRecvBuf, 
					 size_t nMaxSendQue = kInitialSendQue);
  ~TcpConnection();

  EventLoop* getLoop() const { return loop_; }
  const string& name() const { return name_; }
  const InetAddress& localAddress() const { return localAddr_; }
  const InetAddress& peerAddress() const { return peerAddr_; }
  bool connected() const { return state_ == kConnected; }
  bool disconnected() const { return state_ == kDisconnected; }
  bool get_state() { return state_; }
  // return true if success.
  bool getTcpInfo(struct tcp_info*) const;
  string getTcpInfoString() const;
  void	setConnuuid(uint64_t conn_uuid) { conn_uuid_ = conn_uuid; }
  uint64_t getConnuuid() const { return conn_uuid_; }
  void setDefRecvBuf(size_t nDefRecvBuf) { m_nDefRecvBuf = nDefRecvBuf; }
  size_t getDefRecvBuf() { return m_nDefRecvBuf; }
  void setMaxRecvBuf(size_t nMaxRecvBuf) { m_nMaxRecvBuf = nMaxRecvBuf; }
  size_t getMaxRecvBuf() { return m_nMaxRecvBuf; }
  void setMaxSendQue(size_t nMaxSendQue) { m_nMaxSendQue = nMaxSendQue; }
  size_t getMaxSendQue() { return m_nMaxSendQue; }
  // void send(string&& message); // C++11
  int send(const void* message, int len, const boost::any& params, int timeout=0);
  int send(const StringPiece& message, const boost::any& params, int timeout=0);
  // void send(Buffer&& message); // C++11
  int send(Buffer* message, const boost::any& params, int timeout=0);  // this one will swap data
  void shutdown(); // NOT thread safe, no simultaneous calling
  // void shutdownAndForceCloseAfter(double seconds); // NOT thread safe, no simultaneous calling
  void forceClose();
  void forceCloseWithDelay(double seconds);
  void setTcpNoDelay(bool on);
  void suspendRecv(int delay);
  void resumeRecv();
  // reading or not
  void startRead();
  void stopRead();
  bool isReading() const { return reading_; }; // NOT thread safe, may race with start/stopReadInLoop

  void setSockId(int64_t sockid) { m_nSockId = sockid; }
  
  int64_t getSockId() const { return m_nSockId; }
  
  void setHexWeakPtrValue(const std::string& strWeakPtrValue) { strweakptr_ = strWeakPtrValue; }
  
  const string& getHexWeakPtrValue() const { return strweakptr_; }
  
  void tie(const boost::any& _tie)
  { tie_ = _tie; }

  void setContext(const boost::any& context)
  { context_ = context; }

  const boost::any& getContext() const
  { return context_; }

  void setExInfo(const boost::any& exinfo)
  { exinfo_ = exinfo; }
  
  const boost::any& getExInfo() const
  { return exinfo_; }

  void setSessionData(const boost::any& params)
  { session_ = params; }
  
  const boost::any& getSessionData() const
  { return session_; }

  void setNeedCloseFlag() 
  { needclose_ = true; };

  bool bNeedClose() const
  { return needclose_; }
  
  boost::any* getMutableContext()
  { return &context_; }

  void setConnectionCallback(const ConnectionCallback& cb)
  { connectionCallback_ = cb; }

  void setMessageCallback(const MessageCallback& cb)
  { messageCallback_ = cb; }

  void setWriteCompleteCallback(const WriteCompleteCallback& cb)
  {
	  writeCompleteCallback_ = cb;
  }
  void setWriteErrCallback(const WriteErrCallback& cb)
  {
	  writeErrCallback_ = cb;
  }

  void setHighWaterMarkCallback(const HighWaterMarkCallback& cb, size_t highWaterMark)
  { highWaterMarkCallback_ = cb; highWaterMark_ = highWaterMark; }

  void setHighWaterMark(size_t highWaterMark)
  { highWaterMark_ = highWaterMark; }

  /// Internal use only.
  void setCloseCallback(const CloseCallback& cb)
  { closeCallback_ = cb; }

  // called when TcpServer accepts a new connection
  void connectEstablished();   // should be called only once
  // called when TcpServer has removed me from its map
  void connectDestroyed();  // should be called only once
  void connectDestroyedWithLatch(const std::shared_ptr<CountDownLatch>& latch); 

 private:
  enum StateE { kDisconnected, kConnecting, kConnected, kDisconnecting };
  void handleRead(Timestamp receiveTime);
  void handleWrite();
  void handleClose();
  void handleClose2();
  void handleError();
  // void sendInLoop(string&& message);
  void sendInLoop(const StringPiece& message, const boost::any& params, int timeout);
  void sendInLoop(const void* message, size_t len, const boost::any& params, int timeout);
  void shutdownInLoop();
  // void shutdownAndForceCloseInLoop(double seconds);
  void forceCloseInLoop();
  void setState(StateE s) { state_ = s; }
  const char* stateToString() const;
  void startReadInLoop();
  void stopReadInLoop();
  void suspendRecvInLoop(int delay);
  void resumeRecvInLoop();
  // 将所有未发的数据失败回调
  void callbackAllNoSendOutputBuffer();
  // 发送超时回调
  void sendTimeoutCallBack(OUTBUFFER_LIST_IT it);
private:
  bool needclose_;	//是否需要关闭
  string name_;
  uint64_t conn_uuid_;
  std::string strweakptr_;
  EventLoop* loop_;
  StateE state_;  // FIXME: use atomic variable
  bool reading_;
  // we don't expose those classes to client.
  std::unique_ptr<Socket> socket_;
  std::unique_ptr<Channel> channel_;
  const InetAddress localAddr_;
  const InetAddress peerAddr_;
  ConnectionCallback connectionCallback_;
  MessageCallback messageCallback_;
  WriteCompleteCallback writeCompleteCallback_;
  WriteErrCallback writeErrCallback_;
  HighWaterMarkCallback highWaterMarkCallback_;
  CloseCallback closeCallback_;
  std::atomic<size_t> highWaterMark_;
  int64_t m_nSockId;
  size_t m_nDefRecvBuf;
  size_t m_nMaxRecvBuf;
  size_t m_nMaxSendQue;
  Buffer inputBuffer_;
  std::list<OUTBUFFER_PTR> outputBufferList_;
  size_t sendQueSize_;
  boost::any context_;
  boost::any exinfo_;
  boost::any session_;	//store shared_ptr will be better
  TimerId suspendReadTimer_;
  boost::any tie_;
};

typedef std::shared_ptr<TcpConnection> TcpConnectionPtr;

}
}

#endif  // MWNET_MT_NET_TCPCONNECTION_H
