// Use of this source code is governed by a BSD-style license
// that can be found in the License file.
//
// This is an internal header file, you should not include this.

#ifndef MWNET_MT_NET_INSPECT_SYSTEMINSPECTOR_H
#define MWNET_MT_NET_INSPECT_SYSTEMINSPECTOR_H

#include <mwnet_mt/net/inspect/Inspector.h>

namespace mwnet_mt
{
namespace net
{

class SystemInspector : noncopyable
{
 public:
  void registerCommands(Inspector* ins);

  static string overview(HttpRequest::Method, const Inspector::ArgList&);
  static string loadavg(HttpRequest::Method, const Inspector::ArgList&);
  static string version(HttpRequest::Method, const Inspector::ArgList&);
  static string cpuinfo(HttpRequest::Method, const Inspector::ArgList&);
  static string meminfo(HttpRequest::Method, const Inspector::ArgList&);
  static string stat(HttpRequest::Method, const Inspector::ArgList&);
};

}
}

#endif  // MWNET_MT_NET_INSPECT_SYSTEMINSPECTOR_H
