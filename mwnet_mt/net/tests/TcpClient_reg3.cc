// TcpClient destructs in a different thread.

#include <mwnet_mt/base/Logging.h>
#include <mwnet_mt/net/EventLoopThread.h>
#include <mwnet_mt/net/TcpClient.h>

using namespace mwnet_mt;
using namespace mwnet_mt::net;

int main(int argc, char* argv[])
{
  Logger::setLogLevel(Logger::DEBUG);

  EventLoopThread loopThread;
  {
  InetAddress serverAddr("127.0.0.1", 1234); // should succeed
  TcpClient client(loopThread.startLoop(), serverAddr, "TcpClient");
  client.connect();
  CurrentThread::sleepUsec(500 * 1000);  // wait for connect
  client.disconnect();
  }

  CurrentThread::sleepUsec(1000 * 1000);
}
