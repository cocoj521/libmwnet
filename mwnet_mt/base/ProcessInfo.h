// Use of this source code is governed by a BSD-style license
// that can be found in the License file.

// This is a public header file, it must only include public header files.

#ifndef MWNET_MT_BASE_PROCESSINFO_H
#define MWNET_MT_BASE_PROCESSINFO_H

#include <mwnet_mt/base/StringPiece.h>
#include <mwnet_mt/base/Types.h>
#include <mwnet_mt/base/Timestamp.h>
#include <vector>

namespace mwnet_mt
{

namespace ProcessInfo
{
  pid_t pid();
  string pidString();
  uid_t uid();
  string username();
  uid_t euid();
  Timestamp startTime();
  int clockTicksPerSecond();
  int pageSize();
  bool isDebugBuild();  // constexpr

  string hostname();
  string procname();
  StringPiece procname(const string& stat);

  /// read /proc/self/status
  string procStatus();

  /// read /proc/self/stat
  string procStat();

  /// read /proc/self/task/tid/stat
  string threadStat();

  /// readlink /proc/self/exe
  string exePath();

  int openedFiles();
  int maxOpenFiles();

  struct CpuTime
  {
    double userSeconds;
    double systemSeconds;

    CpuTime() : userSeconds(0.0), systemSeconds(0.0) { }
  };
  CpuTime cpuTime();

  int numThreads();
  std::vector<pid_t> threads();
}

}

#endif  // MWNET_MT_BASE_PROCESSINFO_H
