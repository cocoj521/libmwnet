// Use of this source code is governed by a BSD-style license
// that can be found in the License file.

#include <mwnet_mt/net/Poller.h>
#include <mwnet_mt/net/poller/PollPoller.h>
#include <mwnet_mt/net/poller/EPollPoller.h>

#include <stdlib.h>

using namespace mwnet_mt::net;

Poller* Poller::newDefaultPoller(EventLoop* loop)
{
  if (::getenv("MWNET_MT_USE_POLL"))
  {
    return new PollPoller(loop);
  }
  else
  {
    return new EPollPoller(loop);
  }
}
