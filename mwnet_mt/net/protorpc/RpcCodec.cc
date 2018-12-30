// Use of this source code is governed by a BSD-style license
// that can be found in the License file.

#include <mwnet_mt/net/protorpc/RpcCodec.h>

#include <mwnet_mt/base/Logging.h>
#include <mwnet_mt/net/Endian.h>
#include <mwnet_mt/net/TcpConnection.h>

#include <mwnet_mt/net/protorpc/rpc.pb.h>
#include <mwnet_mt/net/protorpc/google-inl.h>

using namespace mwnet_mt;
using namespace mwnet_mt::net;

namespace
{
int ProtobufVersionCheck()
{
	GOOGLE_PROTOBUF_VERIFY_VERSION;
	return 0;
}
int dummy __attribute__ ((unused)) = ProtobufVersionCheck();
}

namespace mwnet_mt
{
namespace net
{
const char rpctag [] = "RPC0";
}
}
