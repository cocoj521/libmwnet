// Use of this source code is governed by a BSD-style license
// that can be found in the License file.
//
// This is a public header file, it must only include public header files.

#ifndef MWNET_MT_NET_PROTORPC_RPCCODEC_H
#define MWNET_MT_NET_PROTORPC_RPCCODEC_H

#include <mwnet_mt/base/Timestamp.h>
#include <mwnet_mt/net/protobuf/ProtobufCodecLite.h>

namespace mwnet_mt
{
namespace net
{

class Buffer;
class TcpConnection;
typedef std::shared_ptr<TcpConnection> TcpConnectionPtr;

class RpcMessage;
typedef std::shared_ptr<RpcMessage> RpcMessagePtr;
extern const char rpctag[];// = "RPC0";

// wire format
//
// Field     Length  Content
//
// size      4-byte  N+8
// "RPC0"    4-byte
// payload   N-byte
// checksum  4-byte  adler32 of "RPC0"+payload
//

typedef ProtobufCodecLiteT<RpcMessage, rpctag> RpcCodec;

}
}

#endif  // MWNET_MT_NET_PROTORPC_RPCCODEC_H
