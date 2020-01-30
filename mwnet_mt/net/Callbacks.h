// Use of this source code is governed by a BSD-style license
// that can be found in the License file.

// This is a public header file, it must only include public header files.

#ifndef MWNET_MT_NET_CALLBACKS_H
#define MWNET_MT_NET_CALLBACKS_H

#include <mwnet_mt/base/Timestamp.h>

#include <functional>
#include <memory>

#include <boost/any.hpp>

namespace mwnet_mt
{

using std::placeholders::_1;
using std::placeholders::_2;
using std::placeholders::_3;

// should really belong to base/Types.h, but <memory> is not included there.

template<typename T>
inline T* get_pointer(const std::shared_ptr<T>& ptr)
{
  return ptr.get();
}

template<typename T>
inline T* get_pointer(const std::unique_ptr<T>& ptr)
{
  return ptr.get();
}

// Adapted from google-protobuf stubs/common.h
// see License in mwnet_mt/base/Types.h
template<typename To, typename From>
inline ::std::shared_ptr<To> down_pointer_cast(const ::std::shared_ptr<From>& f) {
  if (false)
  {
    implicit_cast<From*, To*>(0);
  }

#ifndef NDEBUG
  assert(f == NULL || dynamic_cast<To*>(get_pointer(f)) != NULL);
#endif
  return ::std::static_pointer_cast<To>(f);
}

namespace net
{

// All client visible callbacks go here.

class Buffer;
class TcpConnection;
typedef std::shared_ptr<TcpConnection> TcpConnectionPtr;
typedef std::function<void()> TimerCallback;
typedef std::function<void (const TcpConnectionPtr&)> ConnectionCallback;
typedef std::function<void(const TcpConnectionPtr&, const std::string&)> ConnectionCallback2;
typedef std::function<void (const TcpConnectionPtr&)> CloseCallback;
typedef std::function<void(const TcpConnectionPtr&, const boost::any&)> WriteCompleteCallback;
typedef std::function<void(const TcpConnectionPtr&, const boost::any&, int errcode)> WriteErrCallback;
//typedef std::function<void (const TcpConnectionPtr&)> WriteCompleteCallback;
//typedef std::function<void (const TcpConnectionPtr&, const boost::any& params)> WriteCompleteCallbackEx;
typedef std::function<void (const TcpConnectionPtr&, size_t)> HighWaterMarkCallback;

// the data has been read to (buf, len)
typedef std::function<void (const TcpConnectionPtr&,
                            Buffer*,	//返回上层尚未读取的数据(如果上层一直不读取,会越来越大)
							size_t,		//返回本次网络接收的长度
                            Timestamp)> MessageCallback;

void defaultConnectionCallback(const TcpConnectionPtr& conn);
void defaultMessageCallback(const TcpConnectionPtr& conn,
                            Buffer* buffer,
							size_t,
                            Timestamp receiveTime);

}
}

#endif  // MWNET_MT_NET_CALLBACKS_H
