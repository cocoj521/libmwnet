#include <mwnet_mt/base/ProcessInfo.h>
#include <stdio.h>
#define __STDC_FORMAT_MACROS
#include <inttypes.h>

int main()
{
  printf("pid = %d\n", mwnet_mt::ProcessInfo::pid());
  printf("uid = %d\n", mwnet_mt::ProcessInfo::uid());
  printf("euid = %d\n", mwnet_mt::ProcessInfo::euid());
  printf("start time = %s\n", mwnet_mt::ProcessInfo::startTime().toFormattedString().c_str());
  printf("hostname = %s\n", mwnet_mt::ProcessInfo::hostname().c_str());
  printf("opened files = %d\n", mwnet_mt::ProcessInfo::openedFiles());
  printf("threads = %zd\n", mwnet_mt::ProcessInfo::threads().size());
  printf("num threads = %d\n", mwnet_mt::ProcessInfo::numThreads());
  printf("status = %s\n", mwnet_mt::ProcessInfo::procStatus().c_str());
}
