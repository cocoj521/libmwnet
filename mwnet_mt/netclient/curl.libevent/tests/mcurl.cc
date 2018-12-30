#include "../Curl.h"
#include <mwnet_mt/net/EventLoop.h>
#include <stdio.h>

using namespace mwnet_mt::net;

EventLoop* g_loop = NULL;

void onData(const char* data, int len)
{
  printf("len %d\n", len);
}

void done(curl::Request* c, int code)
{
  printf("done %p %s %d\n", c, c->getEffectiveUrl(), code);
}

void done2(curl::Request* c, int code)
{
  printf("done2 %p %s %d %d\n", c, c->getRedirectUrl(), c->getResponseCode(), code);
  // g_loop->quit();
}

int main(int argc, char* argv[])
{
  EventLoop loop;
  g_loop = &loop;
  //loop.runAfter(30.0, std::bind(&EventLoop::quit, &loop));
  curl::Curl::initialize(curl::Curl::kCURLssl);
  curl::Curl curl(&loop);

  while (1)
  {
  curl::RequestPtr req = curl.getUrl("http://192.169.0.231:1235");
  //req->setDataCallback(onData);
  req->setDoneCallback(done);
  req->request();
  }
  loop.loop();
}
