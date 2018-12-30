// Use of this source code is governed by a BSD-style license
// that can be found in the License file.
//
// This is a public header file, it must only include public header files.

#ifndef MWNET_MT_NET_HTTP_HTTPSERVER_H
#define MWNET_MT_NET_HTTP_HTTPSERVER_H

#include <mwnet_mt/net/TcpServer.h>

namespace mwnet_mt
{
namespace net
{

class HttpRequest;
class HttpResponse;

/// A simple embeddable HTTP server designed for report status of a program.
/// It is not a fully HTTP 1.1 compliant server, but provides minimum features
/// that can communicate with HttpClient and Web browser.
/// It is synchronous, just like Java Servlet.
class HttpServer : noncopyable
{
	static const size_t kInitialSize = 8 + 4 * 1024 * 1024;
 public:
  typedef std::function<void (const HttpRequest&,
                              HttpResponse*)> HttpCallback;

  HttpServer(EventLoop* loop,
  			 const string& name,
             const InetAddress& listenAddr,
             TcpServer::Option option = TcpServer::kNoReusePort,
			 size_t nRecvBuf = kInitialSize, size_t nSendBuf = kInitialSize);

  EventLoop* getLoop() const { return server_.getLoop(); }

  /// Not thread safe, callback be registered before calling start().
  void setHttpCallback(const HttpCallback& cb)
  {
    httpCallback_ = cb;
  }

  void setThreadNum(int numThreads)
  {
    server_.setThreadNum(numThreads);
  }

  void start();

 private:
  void onConnection(const TcpConnectionPtr& conn);
  void onMessage(const TcpConnectionPtr& conn,
                 Buffer* buf,
                 Timestamp receiveTime);
  void onRequest(const TcpConnectionPtr&, const HttpRequest&);

  TcpServer server_;
  HttpCallback httpCallback_;
};

}
}

#endif  // MWNET_MT_NET_HTTP_HTTPSERVER_H
