// Use of this source code is governed by a BSD-style license
// that can be found in the License file.

#ifndef MWNET_MT_BASE_EXCEPTION_H
#define MWNET_MT_BASE_EXCEPTION_H

#include <mwnet_mt/base/Types.h>
#include <exception>

namespace mwnet_mt
{

class Exception : public std::exception
{
 public:
  explicit Exception(const char* what);
  explicit Exception(const string& what);
  virtual ~Exception() throw();
  virtual const char* what() const throw();
  const char* stackTrace() const throw();

 private:
  void fillStackTrace();

  string message_;
  string stack_;
};

}

#endif  // MWNET_MT_BASE_EXCEPTION_H
