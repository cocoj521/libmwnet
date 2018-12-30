#include <mwnet_mt/base/ThreadLocal.h>
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

mwnet_mt::ThreadLocal<Test> testObj1;
mwnet_mt::ThreadLocal<Test> testObj2;

void print()
{
  printf("tid=%d, obj1 %p name=%s\n",
         mwnet_mt::CurrentThread::tid(),
         &testObj1.value(),
         testObj1.value().name().c_str());
  printf("tid=%d, obj2 %p name=%s\n",
         mwnet_mt::CurrentThread::tid(),
         &testObj2.value(),
         testObj2.value().name().c_str());
}

void threadFunc()
{
  print();
  testObj1.value().setName("changed 1");
  testObj2.value().setName("changed 42");
  print();
}

int main()
{
  testObj1.value().setName("main one");
  print();
  mwnet_mt::Thread t1(threadFunc);
  t1.start();
  t1.join();
  testObj2.value().setName("main two");
  print();

  pthread_exit(0);
}
