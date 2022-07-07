#pragma once

#include "support/debug.h"

using namespace std;

template <class Y, class T>
struct __smartptr_compatible
    : std::is_convertible<Y*, T*>
{ };

//template <class U, class V, size_t N>
//struct __smartptr_compatible<U[N], V[]>
//    :    bool_constant<is_same<remove_cv_t<U>, remove_cv_t<V>>::value
//      && is_convertible_v<U*, V*>>
//{ };

template<typename T>
class dshared_ptr {
private:
  std::shared_ptr<T> m_ptr;
public:
  dshared_ptr() : m_ptr(nullptr) { }
  //dshared_ptr(dshared_ptr<T> const& other) : m_ptr(other.m_ptr)
  //{ }
  dshared_ptr(shared_ptr<T> const& ptr) : m_ptr(ptr)
  { }
  dshared_ptr(T* ptr) = delete;
  //dshared_ptr(T* ptr) : m_ptr(ptr)
  //{ }
  void clear() {
    m_ptr = nullptr;
  }
  template <class Y, class = enable_if_t<__smartptr_compatible<Y, T>::value>>
  dshared_ptr(const dshared_ptr<Y>& other) : m_ptr(other.get_shared_ptr())
  { }
  template <class Y, class = enable_if_t<__smartptr_compatible<Y, T>::value>>
  dshared_ptr(dshared_ptr<Y>&& other) : m_ptr(other.get_shared_ptr())
  { }

  shared_ptr<T> get_shared_ptr() const { return m_ptr; }
  //T* get() const { return m_ptr.get(); }
  T& operator*() const { return *m_ptr; }
	T* operator->() const { return m_ptr.get(); }
  operator bool() const { return m_ptr != nullptr; }
	T* get() const { return m_ptr.get(); }


  static dshared_ptr<T> dshared_nullptr() { return dshared_ptr<T>(); }

  friend ostream& operator<<(ostream& os, dshared_ptr<T>& sp)
  {
    os << sp.m_ptr;
    return os;
  }
};

// comparaison operators
template<class T, class U> bool operator==(const dshared_ptr<T>& l, const dshared_ptr<U>& r) throw() // never throws
{
    return (l.get_shared_ptr().get() == r.get_shared_ptr().get());
}
template<class T, class U> bool operator!=(const dshared_ptr<T>& l, const dshared_ptr<U>& r) throw() // never throws
{
    return (l.get_shared_ptr().get() != r.get_shared_ptr().get());
}

template<class T, class U> bool operator<(const dshared_ptr<T>& l, const dshared_ptr<U>& r) = delete;
template<class T, class U> bool operator>(const dshared_ptr<T>& l, const dshared_ptr<U>& r) = delete;
template<class T, class U> bool operator<=(const dshared_ptr<T>& l, const dshared_ptr<U>& r) = delete;
template<class T, class U> bool operator>=(const dshared_ptr<T>& l, const dshared_ptr<U>& r) = delete;

// static cast of shared_ptr
template<class T, class U>
dshared_ptr<T>
static_pointer_cast(const dshared_ptr<U>& ptr) // never throws
{
  return static_pointer_cast<T>(ptr.get_shared_ptr());
}

// dynamic cast of shared_ptr
template<class T, class U>
dshared_ptr<T>
dynamic_pointer_cast(const dshared_ptr<U>& ptr) // never throws
{
    return dynamic_pointer_cast<T>(ptr.get_shared_ptr());
}

template<class T, class... Args>
dshared_ptr<T>
make_dshared(Args&&... args)
{
  return std::make_shared<T>(std::forward<Args>(args)...);
}
