// Use of this source code is governed by a BSD-style license
// that can be found in the License file.

#include <mwnet_mt/base/Thread.h>
#include <mwnet_mt/base/CurrentThread.h>
#include <mwnet_mt/base/CountDownLatch.h>
#include <mwnet_mt/base/Exception.h>
#include <mwnet_mt/base/Logging.h>

#include <type_traits>

#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/prctl.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <linux/unistd.h>

namespace mwnet_mt
{
namespace CurrentThread
{
  __thread int t_cachedTid = 0;
  __thread char t_tidString[32];
  __thread int t_tidStringLength = 6;
  __thread const char* t_threadName = "unknown";
  static_assert(std::is_same<int, pid_t>::value, "pid_t should be int");
}

namespace detail
{

pid_t gettid()
{
  return static_cast<pid_t>(::syscall(SYS_gettid));
}

void afterFork()
{
  mwnet_mt::CurrentThread::t_cachedTid = 0;
  mwnet_mt::CurrentThread::t_threadName = "main";
  CurrentThread::tid();
  // no need to call pthread_atfork(NULL, NULL, &afterFork);
}

class ThreadNameInitializer
{
 public:
  ThreadNameInitializer()
  {
    mwnet_mt::CurrentThread::t_threadName = "main";
    CurrentThread::tid();
    pthread_atfork(NULL, NULL, &afterFork);
  }
};

ThreadNameInitializer init;

struct ThreadData
{
  typedef mwnet_mt::Thread::ThreadFunc ThreadFunc;
  ThreadFunc func_;
  string name_;
  std::weak_ptr<pid_t> wkTid_;
  std::weak_ptr<mwnet_mt::CountDownLatch> wkCDLatch_;
  
  ThreadData(const ThreadFunc& func,
             const string& name,
             const std::shared_ptr<pid_t>& tid,
             const std::shared_ptr<mwnet_mt::CountDownLatch>& cdlatch)
    : func_(func),
      name_(name),
      wkTid_(tid),
      wkCDLatch_(cdlatch)
  { }

  void runInThread()
  {
    pid_t tid = mwnet_mt::CurrentThread::tid();

    std::shared_ptr<pid_t> ptid = wkTid_.lock();
    if (ptid)
    {
      *ptid = tid;
      ptid.reset();
    }
    mwnet_mt::CurrentThread::t_threadName = name_.empty() ? "mwnet_mtThread" : name_.c_str();
    ::prctl(PR_SET_NAME, mwnet_mt::CurrentThread::t_threadName);
    try
    {
      std::shared_ptr<mwnet_mt::CountDownLatch> pcdlatch = wkCDLatch_.lock();
      if (pcdlatch)
      {
        pcdlatch->countDown();
      }
	  
      func_();
      mwnet_mt::CurrentThread::t_threadName = "finished";
    }
    catch (const Exception& ex)
    {
      mwnet_mt::CurrentThread::t_threadName = "crashed";
      fprintf(stderr, "exception caught in Thread %s\n", name_.c_str());
      fprintf(stderr, "reason: %s\n", ex.what());
      fprintf(stderr, "stack trace: %s\n", ex.stackTrace());
	  LOG_SYSERR << "exception caught in Thread:" << name_ << "\n" 
	  			 << "reason: " << ex.what() << "\n"
	  			 << "stack trace: " << ex.stackTrace() << "\n" ;
      //abort();
    }
    catch (const std::exception& ex)
    {
      mwnet_mt::CurrentThread::t_threadName = "crashed";
      fprintf(stderr, "exception caught in Thread %s\n", name_.c_str());
      fprintf(stderr, "reason: %s\n", ex.what());
  	  LOG_SYSERR << "exception caught in Thread:" << name_ << "\n" 
    			 << "reason: " << ex.what() << "\n";
      //abort();
    }
    catch (...)
    {
      mwnet_mt::CurrentThread::t_threadName = "crashed";
      fprintf(stderr, "unknown exception caught in Thread %s\n", name_.c_str());
  	  LOG_SYSERR << "unknown exception caught in Thread:" << name_ << "\n";
      //throw; // rethrow
    }
  }
};

void* startThread(void* obj)
{
  ThreadData* data = static_cast<ThreadData*>(obj);
  data->runInThread();
  delete data;
  return NULL;
}

}
}

using namespace mwnet_mt;

void CurrentThread::cacheTid()
{
  if (t_cachedTid == 0)
  {
    t_cachedTid = detail::gettid();
    t_tidStringLength = snprintf(t_tidString, sizeof t_tidString, "%05d", t_cachedTid);
  }
}

bool CurrentThread::isMainThread()
{
  return tid() == ::getpid();
}

void CurrentThread::sleepUsec(int64_t usec)
{
  struct timespec ts = { 0, 0 };
  ts.tv_sec = static_cast<time_t>(usec / Timestamp::kMicroSecondsPerSecond);
  ts.tv_nsec = static_cast<long>(usec % Timestamp::kMicroSecondsPerSecond * 1000);
  ::nanosleep(&ts, NULL);
}

AtomicInt32 Thread::numCreated_;

Thread::Thread(const ThreadFunc& func, const string& n)
  : started_(false),
    joined_(false),
    pthreadId_(0),
    tid_(new pid_t(0)),
    func_(func),
    name_(n)
{
  setDefaultName();
}

#ifdef __GXX_EXPERIMENTAL_CXX0X__DONOT_USE
Thread::Thread(ThreadFunc&& func, const string& n)
  : started_(false),
    joined_(false),
    pthreadId_(0),
    tid_(new pid_t(0)),
    func_(std::move(func)),
    name_(n)
{
  setDefaultName();
}

#endif

Thread::~Thread()
{
  if (started_ && !joined_)
  {
    pthread_detach(pthreadId_);
  }
}

void Thread::setDefaultName()
{
  int num = numCreated_.incrementAndGet();
  if (name_.empty())
  {
    char buf[32];
    snprintf(buf, sizeof buf, "Thread%d", num);
    name_ = buf;
  }
}

void Thread::start()
{
  assert(!started_);
  started_ = true;
  std::shared_ptr<mwnet_mt::CountDownLatch> pCDLatch(new mwnet_mt::CountDownLatch(1));
  // FIXME: move(func_)
  detail::ThreadData* data = new detail::ThreadData(func_, name_, tid_, pCDLatch);
  if (pthread_create(&pthreadId_, NULL, &detail::startThread, data))
  {
    started_ = false;
    delete data; // or no delete?
    LOG_SYSFATAL << "Failed in pthread_create";
  }
  pCDLatch->wait();
}

int Thread::join()
{
  assert(started_);
  assert(!joined_);
  joined_ = true;
  return pthread_join(pthreadId_, NULL);
}

