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
  LOG_DEBUG << "ctor[" << this << "]";
}

Connector::~Connector()
{
  LOG_DEBUG << "dtor[" << this << "]";
  assert(!channel_);
}

void Connector::start()
{
  connect_ = true;
  loop_->runInLoop(std::bind(&Connector::startInLoop, this)); // FIXME: unsafe
}

void Connector::startInLoop()
{
  loop_->assertInLoopThread();
  //assert(state_ == kDisconnected);
  if (connect_)
  {
    connect();
  }
  else
  {
    LOG_DEBUG << "do not connect";
  }
}

void Connector::stop()
{
  connect_ = false;
  loop_->queueInLoop(std::bind(&Connector::stopInLoop, this)); // FIXME: unsafe
  // FIXME: cancel timer
}

void Connector::stopInLoop()
{
  loop_->assertInLoopThread();
  if (state_ == kConnecting)
  {
      setState(kDisconnected);
      int sockfd = removeAndResetChannel();
      sockets::close(sockfd);
      setState(kDisconnected);
  }
}

#include <errno.h>
#include <fcntl.h>
void Connector::connect()
{
  int sockfd = sockets::createNonblockingOrDie(serverAddr_.family());
    // todo: 设置连接超时， 3秒没连上，就认为失败；系统默认75秒太长
    int ret = sockets::connect(sockfd, serverAddr_.getSockAddr());

  char t_errnobuf[512] = {0};
  int savedErrno = (ret == 0) ? 0 : errno;
  if (savedErrno)
  {
      strerror_r(savedErrno, t_errnobuf, sizeof t_errnobuf);
  }
  //printf("connect:%d-%s\n", savedErrno, t_errnobuf);
  switch (savedErrno)
  {
    case 0:
    case EINPROGRESS:
    case EINTR:
    case EISCONN:
      	connecting(sockfd);
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
  channel_->setWriteCallback(
      std::bind(&Connector::handleWrite, this)); // FIXME: unsafe
  channel_->setErrorCallback(
      std::bind(&Connector::handleError, this)); // FIXME: unsafe

  // channel_->tie(shared_from_this()); is not working,
  // as channel_ is not managed by shared_ptr
  channel_->enableWriting();

}

int Connector::removeAndResetChannel()
{
  channel_->disableAll();
  channel_->remove();
  int sockfd = channel_->fd();
  // Can't reset channel_ here, because we are inside Channel::handleEvent
  loop_->queueInLoop(std::bind(&Connector::resetChannel, this)); // FIXME: unsafe
  return sockfd;
}

void Connector::resetChannel()
{
  channel_.reset();
}

void Connector::handleWrite()
{
  if (state_ == kConnecting)
  {
    int sockfd = removeAndResetChannel();
    int err = sockets::getSocketError(sockfd);
    if (err)
    {
    	sockets::close(sockfd);
		newConnectionCallback_(-1);
    }
    else if (sockets::isSelfConnect(sockfd))
    {
    	sockets::close(sockfd);
		newConnectionCallback_(-1);
    }
    else
    {
      setState(kConnected);
      if (connect_)
      {
        newConnectionCallback_(sockfd);
      }
      else
      {
        sockets::close(sockfd);
		newConnectionCallback_(-1);
      }
    }
  }
  else
  {
    // what happened?
    assert(state_ == kDisconnected);
  }
}

void Connector::handleError()
{
  if (state_ == kConnecting)
  {
    int sockfd = removeAndResetChannel();
    int err = sockets::getSocketError(sockfd);


	if (err){};
	//printf("handleError--------------%d--%d\n", sockfd, err);
	
    sockets::close(sockfd);
    setState(kDisconnected);

	newConnectionCallback_(-1);
  }
  else
  {
    LOG_DEBUG << "do not connect";
  }
}

