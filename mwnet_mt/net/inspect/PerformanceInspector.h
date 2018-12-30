// Use of this source code is governed by a BSD-style license
// that can be found in the License file.
// 
// This is an internal header file, you should not include this.

#ifndef MWNET_MT_NET_INSPECT_PERFORMANCEINSPECTOR_H
#define MWNET_MT_NET_INSPECT_PERFORMANCEINSPECTOR_H

#include <mwnet_mt/net/inspect/Inspector.h>

namespace mwnet_mt
{
namespace net
{

class PerformanceInspector : noncopyable
{
 public:
  void registerCommands(Inspector* ins);

  static string heap(HttpRequest::Method, const Inspector::ArgList&);
  static string growth(HttpRequest::Method, const Inspector::ArgList&);
  static string profile(HttpRequest::Method, const Inspector::ArgList&);
  static string cmdline(HttpRequest::Method, const Inspector::ArgList&);
  static string memstats(HttpRequest::Method, const Inspector::ArgList&);
  static string memhistogram(HttpRequest::Method, const Inspector::ArgList&);
  static string releaseFreeMemory(HttpRequest::Method, const Inspector::ArgList&);

  static string symbol(HttpRequest::Method, const Inspector::ArgList&);
};

}
}

#endif  // MWNET_MT_NET_INSPECT_PERFORMANCEINSPECTOR_H
