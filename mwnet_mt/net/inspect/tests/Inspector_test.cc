#include <mwnet_mt/net/inspect/Inspector.h>
#include <mwnet_mt/net/EventLoop.h>
#include <mwnet_mt/net/EventLoopThread.h>

using namespace mwnet_mt;
using namespace mwnet_mt::net;

int main()
{
  EventLoop loop;
  EventLoopThread t;
  Inspector ins(t.startLoop(), InetAddress(12345), "test");
  loop.loop();
}

