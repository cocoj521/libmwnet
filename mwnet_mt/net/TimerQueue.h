// Use of this source code is governed by a BSD-style license
// that can be found in the License file.
// 
// This is an internal header file, you should not include this.

#ifndef MWNET_MT_NET_TIMERQUEUE_H
#define MWNET_MT_NET_TIMERQUEUE_H

#include <set>
#include <vector>

#include <mwnet_mt/base/Mutex.h>
#include <mwnet_mt/base/Timestamp.h>
#include <mwnet_mt/net/Callbacks.h>
#include <mwnet_mt/net/Channel.h>

namespace mwnet_mt
{
namespace net
{

class EventLoop;
class Timer;
class TimerId;

///
/// A best efforts timer queue.
/// No guarantee that the callback will be on time.
///
class TimerQueue : noncopyable
{
 public:
  TimerQueue(EventLoop* loop);
  ~TimerQueue();

  ///
  /// Schedules the callback to be run at given time,
  /// repeats if @c interval > 0.0.
  ///
  /// Must be thread safe. Usually be called from other threads.
  TimerId addTimer(const TimerCallback& cb,
                   Timestamp when,
                   double interval);
#ifdef __GXX_EXPERIMENTAL_CXX0X__DONOT_USE
  TimerId addTimer(TimerCallback&& cb,
                   Timestamp when,
                   double interval);
#endif

  void cancel(TimerId timerId);

 private:

  // FIXME: use unique_ptr<Timer> instead of raw pointers.
  typedef std::pair<Timestamp, Timer*> Entry;
  typedef std::set<Entry> TimerList;
  typedef std::pair<Timer*, int64_t> ActiveTimer;
  typedef std::set<ActiveTimer> ActiveTimerSet;

  void addTimerInLoop(Timer* timer);
  void cancelInLoop(TimerId timerId);
  // called when timerfd alarms
  void handleRead();
  // move out all expired timers
  std::vector<Entry> getExpired(Timestamp now);
  void reset(const std::vector<Entry>& expired, Timestamp now);

  bool insert(Timer* timer);

  EventLoop* loop_;
  const int timerfd_;
  Channel timerfdChannel_;
  // Timer list sorted by expiration
  TimerList timers_;

  // for cancel()
  ActiveTimerSet activeTimers_;
  bool callingExpiredTimers_; /* atomic */
  ActiveTimerSet cancelingTimers_;
};

}
}
#endif  // MWNET_MT_NET_TIMERQUEUE_H
