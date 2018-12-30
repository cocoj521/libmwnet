#ifndef MWNET_MT_BASE_NONCOPYABLE_H
#define MWNET_MT_BASE_NONCOPYABLE_H

namespace mwnet_mt
{

class noncopyable
{
 protected:
  noncopyable() = default;
  ~noncopyable() = default;

 private:
  noncopyable(const noncopyable&) = delete;
  void operator=(const noncopyable&) = delete;
};

}

#endif  // MWNET_MT_BASE_NONCOPYABLE_H
