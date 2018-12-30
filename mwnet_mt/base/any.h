#ifndef ANY_H
#define ANY_H

#include <typeinfo>
#include <string>

namespace utils
{

  struct bad_any_cast
  {
    std::string _what;

    bad_any_cast(std::string type1, std::string type2);
    
    bad_any_cast();

    std::string what() const;

  };

  class any
  {
  public:

    any()
        :held_(nullptr)
    {}

    template<typename T>
    any(const T& t)
        :held_(new holder<T>(t))
    {}

    template<class T>
    any& operator=(T value)
    {
      delete held_;
      held_ = holder<T>(value).clone();
      return *this;
    }


    any(const any& other);

    ~any();

    template<typename U>
    U value_cast()
    {
      if ((!held_)||(typeid(U) != held_->type_info()))
      {
        const bad_any_cast tmp(typeid(U).name(),held_->type_info().name());
        throw tmp;
      }
      typedef typename std::remove_reference<U>::type A;
      return static_cast<U>(static_cast<holder<A>* >(held_)->t_);
    }

    template<typename U>
    U* pointer_cast()
    {
      if((!held_)||(typeid(U) != held_->type_info()))
      {
        return nullptr;
      }
      return &(static_cast<holder<U>* >(held_)->t_);
    }

    bool empty();

    any& operator=(any& other);

    void swap(any& b);

  private:

    struct base_holder
    {
      virtual ~base_holder(){}
      virtual base_holder* clone() const = 0;
      virtual const std::type_info& type_info() const = 0;
    };

    template<typename T> struct holder : base_holder
    {
      holder(const T& t) : t_(t)
      {}

      const std::type_info& type_info() const
      {
        return typeid(t_);
      }

      virtual base_holder* clone() const
      {
        return new holder(t_);
      }

      T t_;
    };

private:
    base_holder* held_;
};


  template<class T>
  T any_cast(any& a)
  {
    return a.value_cast<T>();
  }

  template<class T>
  T* any_cast(any* a)
  {
    return a->pointer_cast<T>();
  }

  template<class T>
  const T any_cast(const any& a)
  {
    typedef typename std::remove_reference<T>::type A;
    return any_cast<const A&>(const_cast<any&>(a));
  }

  template<class T>
  const T* any_cast(const any* a)
  {
    any* b = const_cast<any*>(a);
    return any_cast<T>(b);
  }

  void swap(any& a, any& b);
}
#endif // ANY_H
