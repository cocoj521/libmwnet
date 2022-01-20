// Use of this source code is governed by a BSD-style license
// that can be found in the License file.

#include <mwnet_mt/net/Poller.h>
#include <mwnet_mt/net/Channel.h>

using namespace mwnet_mt;
using namespace mwnet_mt::net;

Poller::Poller(EventLoop* loop)
  : ownerLoop_(loop)
{
}

Poller::~Poller()
{
}

bool Poller::hasChannel(Channel* channel) const
{
  assertInLoopThread();
  ChannelMap::const_iterator it = channels_.find(channel);
  return it != channels_.end() && it->second == channel;
}

