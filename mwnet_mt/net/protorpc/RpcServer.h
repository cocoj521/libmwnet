// Use of this source code is governed by a BSD-style license
// that can be found in the License file.
//
// This is a public header file, it must only include public header files.

#ifndef MWNET_MT_NET_PROTORPC_RPCSERVER_H
#define MWNET_MT_NET_PROTORPC_RPCSERVER_H

#include <mwnet_mt/net/TcpServer.h>

namespace google {
namespace protobuf {

class Service;

}  // namespace protobuf
}  // namespace google

namespace mwnet_mt
{
namespace net
{

class RpcServer
{
 public:
  RpcServer(EventLoop* loop,
            const InetAddress& listenAddr);

  void setThreadNum(int numThreads)
  {
    server_.setThreadNum(numThreads);
  }

  void registerService(::google::protobuf::Service*);
  void start();

 private:
  void onConnection(const TcpConnectionPtr& conn);

  // void onMessage(const TcpConnectionPtr& conn,
  //                Buffer* buf,
  //                Timestamp time);

  TcpServer server_;
  std::map<std::string, ::google::protobuf::Service*> services_;
};

}
}

#endif  // MWNET_MT_NET_PROTORPC_RPCSERVER_H
