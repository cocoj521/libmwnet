// Use of this source code is governed by a BSD-style license
// that can be found in the License file.

// This is an internal header file, you should not include this.

#ifndef MWNET_MT_NET_POLLER_EPOLLPOLLER_H
#define MWNET_MT_NET_POLLER_EPOLLPOLLER_H

#include <mwnet_mt/net/Poller.h>

#include <vector>

struct epoll_event;

namespace mwnet_mt
{
namespace net
{

///
/// IO Multiplexing with epoll(4).
///
class EPollPoller : public Poller
{
 public:
  EPollPoller(EventLoop* loop);
  virtual ~EPollPoller();

  virtual Timestamp poll(int timeoutMs, ChannelList* activeChannels);
  virtual void updateChannel(Channel* channel);
  virtual void removeChannel(Channel* channel);

 private:
  static const int kInitEventListSize = 16;

  static const char* operationToString(int op);

  void fillActiveChannels(int numEvents,
                          ChannelList* activeChannels) const;
  void update(int operation, Channel* channel);

  typedef std::vector<struct epoll_event> EventList;

  int epollfd_;
  EventList events_;
};

}
}
#endif  // MWNET_MT_NET_POLLER_EPOLLPOLLER_H
