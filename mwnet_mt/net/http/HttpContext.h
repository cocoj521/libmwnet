// Use of this source code is governed by a BSD-style license
// that can be found in the License file.

// This is an internal header file, you should not include this.

#ifndef MWNET_MT_NET_HTTP_HTTPCONTEXT_H
#define MWNET_MT_NET_HTTP_HTTPCONTEXT_H

#include <mwnet_mt/base/copyable.h>

#include <mwnet_mt/net/http/HttpRequest.h>

namespace mwnet_mt
{
namespace net
{

class Buffer;

class HttpContext : public mwnet_mt::copyable
{
 public:
  enum HttpRequestParseState
  {
    kExpectRequestLine,
    kExpectHeaders,
    kExpectBody,
    kGotAll,
  };

  HttpContext()
    : state_(kExpectRequestLine)
  {
  }

  // default copy-ctor, dtor and assignment are fine

  // return false if any error
  bool parseRequest(Buffer* buf, Timestamp receiveTime);

  bool gotAll() const
  { return state_ == kGotAll; }

  void reset()
  {
    state_ = kExpectRequestLine;
    HttpRequest dummy;
    request_.swap(dummy);
  }

  const HttpRequest& request() const
  { return request_; }

  HttpRequest& request()
  { return request_; }

 private:
  bool processRequestLine(const char* begin, const char* end);

  HttpRequestParseState state_;
  HttpRequest request_;
};

}
}

#endif  // MWNET_MT_NET_HTTP_HTTPCONTEXT_H
