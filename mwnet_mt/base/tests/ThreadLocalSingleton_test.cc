#include <mwnet_mt/base/ThreadLocalSingleton.h>
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

void threadFunc(const char* changeTo)
{
  printf("tid=%d, %p name=%s\n",
         mwnet_mt::CurrentThread::tid(),
         &mwnet_mt::ThreadLocalSingleton<Test>::instance(),
         mwnet_mt::ThreadLocalSingleton<Test>::instance().name().c_str());
  mwnet_mt::ThreadLocalSingleton<Test>::instance().setName(changeTo);
  printf("tid=%d, %p name=%s\n",
         mwnet_mt::CurrentThread::tid(),
         &mwnet_mt::ThreadLocalSingleton<Test>::instance(),
         mwnet_mt::ThreadLocalSingleton<Test>::instance().name().c_str());

  // no need to manually delete it
  // mwnet_mt::ThreadLocalSingleton<Test>::destroy();
}

int main()
{
  mwnet_mt::ThreadLocalSingleton<Test>::instance().setName("main one");
  mwnet_mt::Thread t1(std::bind(threadFunc, "thread1"));
  mwnet_mt::Thread t2(std::bind(threadFunc, "thread2"));
  t1.start();
  t2.start();
  t1.join();
  printf("tid=%d, %p name=%s\n",
         mwnet_mt::CurrentThread::tid(),
         &mwnet_mt::ThreadLocalSingleton<Test>::instance(),
         mwnet_mt::ThreadLocalSingleton<Test>::instance().name().c_str());
  t2.join();

  pthread_exit(0);
}
