// Use of this source code is governed by a BSD-style license
// that can be found in the License file.

#include <mwnet_mt/net/Timer.h>

using namespace mwnet_mt;
using namespace mwnet_mt::net;

AtomicInt64 Timer::s_numCreated_;

void Timer::restart(Timestamp now)
{
  if (repeat_)
  {
    expiration_ = addTime(now, interval_);
  }
  else
  {
    expiration_ = Timestamp::invalid();
  }
}
