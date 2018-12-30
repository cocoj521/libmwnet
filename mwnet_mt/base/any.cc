#include "any.h"

namespace utils
{

  bad_any_cast::bad_any_cast(std::string type1, std::string type2)
  {
    _what = type2 + "->" + type1;
  }

  bad_any_cast::bad_any_cast()
  {
    _what = "";
  }

  std::string bad_any_cast::what() const
  {
    return _what;
  }

  any::any(const any& other)
  {
    held_ = other.held_ ? other.held_->clone() : 0;
  }

  any::~any()
  {
    delete held_;
  }

  bool any::empty()
  {
    return !(held_);
  }

  any& any::operator=(any& other)
  {
    any(other).swap(*this);
    return *this;
  }

  void any::swap(any& b)
  {
    std::swap(held_, b.held_);
  }

  void swap(any& a, any& b)
  {
    a.swap(b);
  }
}
