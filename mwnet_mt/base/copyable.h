#ifndef MWNET_MT_BASE_COPYABLE_H
#define MWNET_MT_BASE_COPYABLE_H

namespace mwnet_mt
{

/// A tag class emphasises the objects are copyable.
/// The empty base class optimization applies.
/// Any derived class of copyable should be a value type.
class copyable
{
 protected:
  copyable() = default;
  ~copyable() = default;
};

};

#endif  // MWNET_MT_BASE_COPYABLE_H
