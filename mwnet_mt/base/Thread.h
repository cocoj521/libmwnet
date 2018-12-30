// Use of this source code is governed by a BSD-style license
// that can be found in the License file.

#ifndef MWNET_MT_BASE_THREAD_H
#define MWNET_MT_BASE_THREAD_H

#include <mwnet_mt/base/Atomic.h>
#include <mwnet_mt/base/Types.h>

#include <functional>
#include <memory>
#include <pthread.h>

namespace mwnet_mt
{

class Thread : noncopyable
{
 public:
  typedef std::function<void ()> ThreadFunc;

  explicit Thread(const ThreadFunc&, const string& name = string());
#ifdef __GXX_EXPERIMENTAL_CXX0X__DONOT_USE
  explicit Thread(ThreadFunc&&, const string& name = string());
#endif
  // FIXME: make it movable in C++11
  ~Thread();

  void start();
  int join(); // return pthread_join()

  bool started() const { return started_; }
  // pthread_t pthreadId() const { return pthreadId_; }
  pid_t tid() const { return *tid_; }
  const string& name() const { return name_; }

  static int numCreated() { return numCreated_.get(); }

 private:
  void setDefaultName();

  bool       started_;
  bool       joined_;
  pthread_t  pthreadId_;
  std::shared_ptr<pid_t> tid_;
  ThreadFunc func_;
  string     name_;

  static AtomicInt32 numCreated_;
};

}
#endif
