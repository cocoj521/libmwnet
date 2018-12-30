#include <mwnet_mt/base/Singleton.h>
#include <mwnet_mt/base/CurrentThread.h>
#include <mwnet_mt/base/ThreadLocal.h>
#include <mwnet_mt/base/Thread.h>

#include <stdio.h>
#include <unistd.h>

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

#define STL mwnet_mt::Singleton<mwnet_mt::ThreadLocal<Test> >::instance().value()

void print()
{
  printf("tid=%d, %p name=%s\n",
         mwnet_mt::CurrentThread::tid(),
         &STL,
         STL.name().c_str());
}

void threadFunc(const char* changeTo)
{
  print();
  STL.setName(changeTo);
  sleep(1);
  print();
}

int main()
{
  STL.setName("main one");
  mwnet_mt::Thread t1(std::bind(threadFunc, "thread1"));
  mwnet_mt::Thread t2(std::bind(threadFunc, "thread2"));
  t1.start();
  t2.start();
  t1.join();
  print();
  t2.join();
  pthread_exit(0);
}
