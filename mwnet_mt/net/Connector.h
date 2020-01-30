// Use of this source code is governed by a BSD-style license
// that can be found in the License file.

// This is an internal header file, you should not include this.

#ifndef MWNET_MT_NET_CONNECTOR_H
#define MWNET_MT_NET_CONNECTOR_H

#include <mwnet_mt/net/InetAddress.h>
#include <mwnet_mt/net/TimerId.h>
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
  // sockErr��ʾ�����socket����,������ϵͳ���� 0:��ɹ�,����:��ʾʧ��
  // otherErr��ʾ�������� 0:�ɹ�,1:���ӳ�ʱ,2:������,3:����ֹͣ����
  typedef std::function<void (int sockfd, int sockErr, int otherErr)> NewConnectionCallback;

  Connector(EventLoop* loop, const InetAddress& serverAddr);
  ~Connector();

  void setNewConnectionCallback(const NewConnectionCallback& cb)
  { newConnectionCallback_ = cb; }

  void start(int timeout_s=0);  // can be called in any thread,if timeout_s=0 use sysdefined timeout 75s
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
  const char* stateToString() const
  {
	  switch (state_)
	  {
	  case kDisconnected:
		  return "kDisconnected";
	  case kConnecting:
		  return "kConnecting";
	  case kConnected:
		  return "kConnected";
	  default:
		  return "unknown state";
	  }
  }
  void setState(States s) { state_ = s; }
  void startInLoop();
  void stopInLoop();
  bool connect(int& nErrCode);//������true,connect����첽�ص�;������false,���������лص�
  void connecting(int sockfd);
  void handleWrite();
  void handleError();
  void handleConnTimeOut();
  void handleConnTimeOutInLoop();
  void retry(int sockfd);
  int removeAndResetChannel();
  void resetChannel();

  EventLoop* loop_;
  InetAddress serverAddr_;
  bool connect_; // atomic
  States state_;  // FIXME: use atomic variable
  std::unique_ptr<Channel> channel_;
  NewConnectionCallback newConnectionCallback_;
  int conn_timeout_;
  TimerId conn_timeout_timer_;
};

}
}

#endif  // MWNET_MT_NET_CONNECTOR_H
