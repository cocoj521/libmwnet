#include <mwnet_mt/base/Singleton.h>
#include <mwnet_mt/base/CurrentThread.h>
#include <mwnet_mt/base/Thread.h>

#include <stdio.h>

class Test : mwnet_mt::noncopyable
{
 public:
  Test()
  {
    printf("tid=%d, constructing %p\n", mwnet_mt::CurrentThread::tid(), this);
  }

  ~Test()
  {
    printf("tid=%d, destructing %p %s\n", mwnet_mt::CurrentThread::tid(), this, name_.c_str());
  }

  const mwnet_mt::string& name() const { return name_; }
  void setName(const mwnet_mt::string& n) { name_ = n; }

 private:
  mwnet_mt::string name_;
};

class TestNoDestroy : mwnet_mt::noncopyable
{
 public:
  // Tag member for Singleton<T>
  void no_destroy();

  TestNoDestroy()
  {
    printf("tid=%d, constructing TestNoDestroy %p\n", mwnet_mt::CurrentThread::tid(), this);
  }

  ~TestNoDestroy()
  {
    printf("tid=%d, destructing TestNoDestroy %p\n", mwnet_mt::CurrentThread::tid(), this);
  }
};

void threadFunc()
{
  printf("tid=%d, %p name=%s\n",
         mwnet_mt::CurrentThread::tid(),
         &mwnet_mt::Singleton<Test>::instance(),
         mwnet_mt::Singleton<Test>::instance().name().c_str());
  mwnet_mt::Singleton<Test>::instance().setName("only one, changed");
}

int main()
{
  mwnet_mt::Singleton<Test>::instance().setName("only one");
  mwnet_mt::Thread t1(threadFunc);
  t1.start();
  t1.join();
  printf("tid=%d, %p name=%s\n",
         mwnet_mt::CurrentThread::tid(),
         &mwnet_mt::Singleton<Test>::instance(),
         mwnet_mt::Singleton<Test>::instance().name().c_str());
  mwnet_mt::Singleton<TestNoDestroy>::instance();
  printf("with valgrind, you should see %zd-byte memory leak.\n", sizeof(TestNoDestroy));
}
