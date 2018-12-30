// Use of this source code is governed by a BSD-style license
// that can be found in the License file.

#include <mwnet_mt/net/http/HttpServer.h>

#include <mwnet_mt/base/Logging.h>
#include <mwnet_mt/net/http/HttpContext.h>
#include <mwnet_mt/net/http/HttpRequest.h>
#include <mwnet_mt/net/http/HttpResponse.h>

using namespace mwnet_mt;
using namespace mwnet_mt::net;

namespace mwnet_mt
{
namespace net
{
namespace detail
{

void defaultHttpCallback(const HttpRequest&, HttpResponse* resp)
{
  resp->setStatusCode(HttpResponse::k404NotFound);
  resp->setStatusMessage("Not Found");
  resp->setCloseConnection(true);
}

}
}
}

HttpServer::HttpServer(EventLoop* loop,
						const string& name,
                       const InetAddress& listenAddr,
                       TcpServer::Option option,
					   size_t nRecvBuf, size_t nSendBuf)
					   : server_(loop, name, listenAddr, option, nRecvBuf, nSendBuf),
    httpCallback_(detail::defaultHttpCallback)
{
  server_.setConnectionCallback(
      std::bind(&HttpServer::onConnection, this, _1));
  server_.setMessageCallback(
      std::bind(&HttpServer::onMessage, this, _1, _2, _3));
}

void HttpServer::start()
{
  // LOG_WARN << "HttpServer[" << server_.name()
    // << "] starts listenning on " << server_.ipPort();
  server_.start();
}

void HttpServer::onConnection(const TcpConnectionPtr& conn)
{
  if (conn->connected())
  {
    conn->setContext(HttpContext());
  }
}

void HttpServer::onMessage(const TcpConnectionPtr& conn,
                           Buffer* buf,
                           Timestamp receiveTime)
{
  HttpContext* context = boost::any_cast<HttpContext>(conn->getMutableContext());
  
  //LOG_WARN << "http request:\r\n" << buf->peek();
  
  if (!context->parseRequest(buf, receiveTime))
  {
    //LOG_WARN << "response 400==============";
    conn->send("HTTP/1.1 400 Bad Request\r\n\r\n");
    conn->shutdown();
  }

  if (context->gotAll())
  {
    onRequest(conn, context->request());
    context->reset();
  }
}

void HttpServer::onRequest(const TcpConnectionPtr& conn, const HttpRequest& req)
{
  const string& connection = req.getHeader("Connection");
  bool close = connection == "close" ||
    (req.getVersion() == HttpRequest::kHttp10 && connection != "Keep-Alive");
  HttpResponse response(close);
  httpCallback_(req, &response);
  Buffer buf;
  response.appendToBuffer(&buf);
  conn->send(&buf);
  if (response.closeConnection())
  {
    conn->shutdown();
  }
}

