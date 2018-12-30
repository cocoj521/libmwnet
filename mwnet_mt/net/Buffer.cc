// Use of this source code is governed by a BSD-style license
// that can be found in the License file.

#include <mwnet_mt/net/Buffer.h>

#include <mwnet_mt/net/SocketsOps.h>

#include <errno.h>
#include <sys/uio.h>

using namespace mwnet_mt;
using namespace mwnet_mt::net;

const char Buffer::kCRLF[] = "\r\n";

const size_t Buffer::kCheapPrepend;
const size_t Buffer::kInitialSize;
const size_t Buffer::kExtraBufSize;

ssize_t Buffer::readFd(int fd, int* savedErrno)
{	
  // saved an ioctl()/FIONREAD call to tell how much to read
  char extrabuf[kExtraBufSize];
  struct iovec vec[2];
  const size_t writable = writableBytes();
  vec[0].iov_base = begin()+writerIndex_;
  vec[0].iov_len  = writable;
  vec[1].iov_base = extrabuf;
  vec[1].iov_len  = kExtraBufSize;
  // when there is enough space in this buffer, don't read into extrabuf.
  // when extrabuf is used, we read 128k-1 bytes at most.
  const int iovcnt = (writable < kExtraBufSize) ? 2 : 1;
  const ssize_t n = sockets::readv(fd, vec, iovcnt);
  if (n < 0)
  {
    *savedErrno = errno;
  }
  else if (implicit_cast<size_t>(n) <= writable)
  {
    writerIndex_ += n;
  }
  else
  {
    writerIndex_ = buffer_.size();
    append(extrabuf, n - writable);
  }
  return n;
}

