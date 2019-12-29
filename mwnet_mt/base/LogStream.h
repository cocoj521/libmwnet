#ifndef MWNET_MT_BASE_LOGSTREAM_H
#define MWNET_MT_BASE_LOGSTREAM_H

#include <mwnet_mt/base/StringPiece.h>
#include <mwnet_mt/base/Types.h>
#include <assert.h>
#include <string.h> // memcpy
#include <memory>

namespace mwnet_mt
{

namespace detail
{

const int kSmallBuffer = 16*1024;
const int kLargeBuffer = 4*1024*1024;

template<int SIZE>
class FixedBuffer : noncopyable
{
 public:
  FixedBuffer()
    : cur_(data_)
  {
    setCookie(cookieStart);
  }

  ~FixedBuffer()
  {
    setCookie(cookieEnd);
  }

  void append(const char* /*restrict*/ buf, size_t len)
  {
    // FIXME: append partially
    if (implicit_cast<size_t>(avail()) > len)
    {
      memcpy(cur_, buf, len);
      cur_ += len;
    }
  }

  const char* data() const { return data_; }
  int length() const { return static_cast<int>(cur_ - data_); }

  // write to data_ directly
  char* current() { return cur_; }
  int avail() const { return static_cast<int>(end() - cur_); }
  void add(size_t len) { cur_ += len; }

  void reset() { cur_ = data_; }
  void bzero() { ::bzero(data_, sizeof data_); }

  // for used by GDB
  const char* debugString();
  void setCookie(void (*cookie)()) { cookie_ = cookie; }
  // for used by unit test
  string toString() const { return string(data_, length()); }
  StringPiece toStringPiece() const { return StringPiece(data_, length()); }

 private:
  const char* end() const { return data_ + sizeof data_; }
  // Must be outline function for cookies.
  static void cookieStart();
  static void cookieEnd();

  void (*cookie_)();
  char data_[SIZE];
  char* cur_;
};

}

class LogStream : noncopyable
{
  typedef LogStream self;
 public:
  typedef detail::FixedBuffer<detail::kSmallBuffer> Buffer;
  LogStream()
  {
	  p_buffer_.reset(new string());
	  p_buffer_->reserve(detail::kSmallBuffer);
  }
  self& operator<<(bool v)
  {
	  p_buffer_->append(v ? "1" : "0", 1);
	  return *this;
  }

  self& operator<<(short);
  self& operator<<(unsigned short);
  self& operator<<(int);
  self& operator<<(unsigned int);
  self& operator<<(long);
  self& operator<<(unsigned long);
  self& operator<<(long long);
  self& operator<<(unsigned long long);

  self& operator<<(const void*);

  self& operator<<(float v)
  {
    *this << static_cast<double>(v);
    return *this;
  }
  self& operator<<(double);
  // self& operator<<(long double);

  self& operator<<(char v)
  {
	  p_buffer_->append(&v, 1);
	  return *this;
  }

  // self& operator<<(signed char);
  // self& operator<<(unsigned char);

  self& operator<<(const char* str)
  {
    if (str)
    {
		p_buffer_->append(str, strlen(str));
    }
    else
    {
		p_buffer_->append("(null)", 6);
    }
    return *this;
  }

  self& operator<<(const unsigned char* str)
  {
    return operator<<(reinterpret_cast<const char*>(str));
  }

  self& operator<<(const string& v)
  {
	  p_buffer_->append(v.c_str(), v.size());
	  return *this;
  }

  self& operator<<(const StringPiece& v)
  {
	  p_buffer_->append(v.data(), v.size());
	  return *this;
  }

  self& operator<<(const Buffer& v)
  {
    *this << v.toStringPiece();
    return *this;
  }

  void append(const char* data, int len) { p_buffer_->append(data, len); }
  const std::shared_ptr<string>& buffer() const { return p_buffer_; }
  void resetBuffer() { p_buffer_->resize(0); }

 private:
  void staticCheck();

  template<typename T>
  void formatInteger(T);

  std::shared_ptr<string> p_buffer_;

  static const int kMaxNumericSize = 32;
};

class Fmt // : noncopyable
{
 public:
  template<typename T>
  Fmt(const char* fmt, T val);

  const char* data() const { return buf_; }
  int length() const { return length_; }

 private:
  char buf_[32];
  int length_;
};

inline LogStream& operator<<(LogStream& s, const Fmt& fmt)
{
  s.append(fmt.data(), fmt.length());
  return s;
}

}
#endif  // MWNET_MT_BASE_LOGSTREAM_H

