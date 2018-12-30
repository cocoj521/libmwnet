#include <mwnet_mt/base/Logging.h>
#include <mwnet_mt/net/Channel.h>
#include <mwnet_mt/net/EventLoop.h>

#include <functional>
#include <map>

#include <stdio.h>
#include <unistd.h>
#include <sys/timerfd.h>

using namespace mwnet_mt;
using namespace mwnet_mt::net;

void print(const char* msg)
{
  static std::map<const char*, Timestamp> lasts;
  Timestamp& last = lasts[msg];
  Timestamp now = Timestamp::now();
  printf("%s tid %d %s delay %f\n", now.toString().c_str(), CurrentThread::tid(),
         msg, timeDifference(now, last));
  last = now;
}

namespace mwnet_mt
{
namespace net
{
namespace detail
{
int createTimerfd();
void readTimerfd(int timerfd, Timestamp now);
}
}
}

// Use relative time, immunized to wall clock changes.
class PeriodicTimer
{
 public:
  PeriodicTimer(EventLoop* loop, double interval, const TimerCallback& cb)
    : loop_(loop),
      timerfd_(mwnet_mt::net::detail::createTimerfd()),
      timerfdChannel_(loop, timerfd_),
      interval_(interval),
      cb_(cb)
  {
    timerfdChannel_.setReadCallback(
        std::bind(&PeriodicTimer::handleRead, this));
    timerfdChannel_.enableReading();
  }

  void start()
  {
    struct itimerspec spec;
    bzero(&spec, sizeof spec);
    spec.it_interval = toTimeSpec(interval_);
    spec.it_value = spec.it_interval;
    int ret = ::timerfd_settime(timerfd_, 0 /* relative timer */, &spec, NULL);
    if (ret)
    {
      LOG_SYSERR << "timerfd_settime()";
    }
  }

  ~PeriodicTimer()
  {
    timerfdChannel_.disableAll();
    timerfdChannel_.remove();
    ::close(timerfd_);
  }

 private:
  void handleRead()
  {
    loop_->assertInLoopThread();
    mwnet_mt::net::detail::readTimerfd(timerfd_, Timestamp::now());
    if (cb_)
      cb_();
  }

  static struct timespec toTimeSpec(double seconds)
  {
    struct timespec ts;
    bzero(&ts, sizeof ts);
    const int64_t kNanoSecondsPerSecond = 1e9;
    const int kMinInterval = 1e5;
    int64_t nanoseconds = static_cast<int64_t>(seconds * kNanoSecondsPerSecond);
    if (nanoseconds < kMinInterval)
      nanoseconds = kMinInterval;
    ts.tv_sec = static_cast<time_t>(nanoseconds / kNanoSecondsPerSecond);
    ts.tv_nsec = static_cast<long>(nanoseconds % kNanoSecondsPerSecond);
    return ts;
  }

  EventLoop* loop_;
  const int timerfd_;
  Channel timerfdChannel_;
  const double interval_; // in seconds
  TimerCallback cb_;
};

int main(int argc, char* argv[])
{
  LOG_INFO << "pid = " << getpid() << ", tid = " << CurrentThread::tid()
           << " Try adjusting the wall clock, see what happens.";
  EventLoop loop;
  PeriodicTimer timer(&loop, 1, std::bind(print, "PeriodicTimer"));
  timer.start();
  loop.runEvery(1, std::bind(print, "EventLoop::runEvery"));
  loop.loop();
}
