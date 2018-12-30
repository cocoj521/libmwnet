#include <mwnet_mt/base/Thread.h>
#include <mwnet_mt/base/CurrentThread.h>

#include <string>
#include <stdio.h>
#include <unistd.h>

void mysleep(int seconds)
{
  timespec t = { seconds, 0 };
  nanosleep(&t, NULL);
}

void threadFunc()
{
  printf("tid=%d\n", mwnet_mt::CurrentThread::tid());
}

void threadFunc2(int x)
{
  printf("tid=%d, x=%d\n", mwnet_mt::CurrentThread::tid(), x);
}

void threadFunc3()
{
  printf("tid=%d\n", mwnet_mt::CurrentThread::tid());
  mysleep(1);
}

class Foo
{
 public:
  explicit Foo(double x)
    : x_(x)
  {
  }

  void memberFunc()
  {
    printf("tid=%d, Foo::x_=%f\n", mwnet_mt::CurrentThread::tid(), x_);
  }

  void memberFunc2(const std::string& text)
  {
    printf("tid=%d, Foo::x_=%f, text=%s\n", mwnet_mt::CurrentThread::tid(), x_, text.c_str());
  }

 private:
  double x_;
};

int main()
{
  printf("pid=%d, tid=%d\n", ::getpid(), mwnet_mt::CurrentThread::tid());

  mwnet_mt::Thread t1(threadFunc);
  t1.start();
  t1.join();

  mwnet_mt::Thread t2(std::bind(threadFunc2, 42),
                   "thread for free function with argument");
  t2.start();
  t2.join();

  Foo foo(87.53);
  mwnet_mt::Thread t3(std::bind(&Foo::memberFunc, &foo),
                   "thread for member function without argument");
  t3.start();
  t3.join();

  mwnet_mt::Thread t4(std::bind(&Foo::memberFunc2, std::ref(foo), std::string("Shuo Chen")));
  t4.start();
  t4.join();

  {
    mwnet_mt::Thread t5(threadFunc3);
    t5.start();
    // t5 may destruct eariler than thread creation.
  }
  mysleep(2);
  {
    mwnet_mt::Thread t6(threadFunc3);
    t6.start();
    mysleep(2);
    // t6 destruct later than thread creation.
  }
  sleep(2);
  printf("number of created threads %d\n", mwnet_mt::Thread::numCreated());
}
