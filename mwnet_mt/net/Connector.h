// Use of this source code is governed by a BSD-style license
// that can be found in the License file.

// This is an internal header file, you should not include this.

#ifndef MWNET_MT_NET_CONNECTOR_H
#define MWNET_MT_NET_CONNECTOR_H

#include <mwnet_mt/net/InetAddress.h>

#include <functional>
#include <memory>

namespace mwnet_mt
{
namespace net
{

class Channel;
class EventLoop;

class Connector : noncopyable,
                  public std::enable_shared_from_this<Connector>
{
 public:
  typedef std::function<void (int sockfd)> NewConnectionCallback;

  Connector(EventLoop* loop, const InetAddress& serverAddr);
  ~Connector();

  void setNewConnectionCallback(const NewConnectionCallback& cb)
  { newConnectionCallback_ = cb; }

  void start();  // can be called in any thread
  void restart();  // must be called in loop thread
  void stop();  // can be called in any thread
  void enableRetry() {}

  void setNewAddr(const InetAddress& serverAddr)
  {
  	serverAddr_ = serverAddr;
  }
  
  const InetAddress& serverAddress() const { return serverAddr_; }

 private:
  enum States { kDisconnected, kConnecting, kConnected };

  void setState(States s) { state_ = s; }
  void startInLoop();
  void stopInLoop();
  void connect();
  void connecting(int sockfd);
  void handleWrite();
  void handleError();
  void retry(int sockfd);
  int removeAndResetChannel();
  void resetChannel();

  EventLoop* loop_;
  InetAddress serverAddr_;
  bool connect_; // atomic
  States state_;  // FIXME: use atomic variable
  std::unique_ptr<Channel> channel_;
  NewConnectionCallback newConnectionCallback_;
};

}
}

#endif  // MWNET_MT_NET_CONNECTOR_H
