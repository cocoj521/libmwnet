// Use of this source code is governed by a BSD-style license
// that can be found in the License file.

#include <mwnet_mt/net/Poller.h>
#include <mwnet_mt/net/poller/EPollPoller.h>

#include <stdlib.h>

using namespace mwnet_mt::net;

Poller* Poller::newDefaultPoller(EventLoop* loop)
{
   return new EPollPoller(loop);
}
