// Use of this source code is governed by a BSD-style license
// that can be found in the License file.
//
// This is a public header file, it must only include public header files.

#ifndef MWNET_MT_NET_INSPECT_INSPECTOR_H
#define MWNET_MT_NET_INSPECT_INSPECTOR_H

#include <mwnet_mt/base/Mutex.h>
#include <mwnet_mt/base/Singleton.h>
#include <mwnet_mt/net/EventLoop.h>
#include <mwnet_mt/net/EventLoopThread.h>
#include <mwnet_mt/net/http/HttpRequest.h>
#include <mwnet_mt/net/http/HttpServer.h>

#include <map>

namespace mwnet_mt
{
namespace net
{

class ProcessInspector;
class PerformanceInspector;
class SystemInspector;

// An internal inspector of the running process, usually a singleton.
// Better to run in a seperated thread, as some method may block for seconds
class Inspector : noncopyable
{
 public:
  typedef std::vector<string> ArgList;
  typedef std::function<string (HttpRequest::Method, const ArgList& args)> Callback;
  ///Inspector(EventLoop* loop,
  ///          const InetAddress& httpAddr,
  ///          const string& name);

  Inspector();

  ~Inspector();

  static Inspector & instance() { return Singleton<Inspector>::instance(); }
  
  void init(const InetAddress& httpAddr, const string& name);
 
  /// Add a Callback for handling the special uri : /mudule/command
  void add(const string& module,
           const string& command,
           const Callback& cb,
           const string& help);
  void remove(const string& module, const string& command);

 private:
  typedef std::map<string, Callback> CommandList;
  typedef std::map<string, string> HelpList;

  void start();
  void onRequest(const HttpRequest& req, HttpResponse* resp);

  /// HttpServer server_;
  EventLoop* loop_;
  std::unique_ptr<EventLoopThread> loopThread_;
  std::unique_ptr<HttpServer> server_;
  std::unique_ptr<ProcessInspector> processInspector_;
  std::unique_ptr<PerformanceInspector> performanceInspector_;
  std::unique_ptr<SystemInspector> systemInspector_;
  MutexLock mutex_;
  std::map<string, CommandList> modules_;
  std::map<string, HelpList> helps_;
};

}
}

#endif  // MWNET_MT_NET_INSPECT_INSPECTOR_H
