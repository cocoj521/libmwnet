// Use of this source code is governed by a BSD-style license
// that can be found in the License file.

// This is a public header file, it must only include public header files.

#ifndef MWNET_MT_NET_TIMERID_H
#define MWNET_MT_NET_TIMERID_H

#include <mwnet_mt/base/copyable.h>

namespace mwnet_mt
{
namespace net
{

class Timer;

///
/// An opaque identifier, for canceling Timer.
///
class TimerId : public mwnet_mt::copyable
{
 public:
  TimerId()
    : timer_(NULL),
      sequence_(0)
  {
  }

  TimerId(Timer* timer, int64_t seq)
    : timer_(timer),
      sequence_(seq)
  {
  }

  // default copy-ctor, dtor and assignment are okay

  friend class TimerQueue;
  
  int64_t getSeq() { return sequence_; }  // add by dengyu
  Timer* getTimer() { return timer_; }    // add by dengyu
 
 private:
  Timer* timer_;
  int64_t sequence_;
};

}
}

#endif  // MWNET_MT_NET_TIMERID_H
