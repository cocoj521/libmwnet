// Copyright 2010, Shuo Chen.  All rights reserved.
// http://code.google.com/p/mwnet_mt/
//
// Use of this source code is governed by a BSD-style license
// that can be found in the License file.

// Author: Shuo Chen (chenshuo at chenshuo dot com)
//
// This is an internal header file, you should not include this.

#ifndef MWNET_MT_NET_INSPECT_PROCESSINSPECTOR_H
#define MWNET_MT_NET_INSPECT_PROCESSINSPECTOR_H

#include <mwnet_mt/net/inspect/Inspector.h>

namespace mwnet_mt
{
namespace net
{

class ProcessInspector : noncopyable
{
 public:
  void registerCommands(Inspector* ins);

  static string overview(HttpRequest::Method, const Inspector::ArgList&);
  static string pid(HttpRequest::Method, const Inspector::ArgList&);
  static string procStatus(HttpRequest::Method, const Inspector::ArgList&);
  static string openedFiles(HttpRequest::Method, const Inspector::ArgList&);
  static string threads(HttpRequest::Method, const Inspector::ArgList&);

  static string username_;
};

}
}

#endif  // MWNET_MT_NET_INSPECT_PROCESSINSPECTOR_H
