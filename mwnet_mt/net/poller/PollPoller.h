// Use of this source code is governed by a BSD-style license
// that can be found in the License file.

// This is an internal header file, you should not include this.

#ifndef MWNET_MT_NET_POLLER_POLLPOLLER_H
#define MWNET_MT_NET_POLLER_POLLPOLLER_H

#include <mwnet_mt/net/Poller.h>

#include <vector>

struct pollfd;

namespace mwnet_mt
{
namespace net
{

///
/// IO Multiplexing with poll(2).
///
class PollPoller : public Poller
{
 public:

  PollPoller(EventLoop* loop);
  virtual ~PollPoller();

  virtual Timestamp poll(int timeoutMs, ChannelList* activeChannels);
  virtual void updateChannel(Channel* channel);
  virtual void removeChannel(Channel* channel);

 private:
  void fillActiveChannels(int numEvents,
                          ChannelList* activeChannels) const;

  typedef std::vector<struct pollfd> PollFdList;
  PollFdList pollfds_;
};

}
}
#endif  // MWNET_MT_NET_POLLER_POLLPOLLER_H
