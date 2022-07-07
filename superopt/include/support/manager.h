#pragma once
#include <unordered_map>
#include <cassert>
#include <memory>
#include <sstream>
#include <iostream>
#include <typeinfo>
#include <cxxabi.h>


#include "support/timers.h"
#include "support/mytimer.h"
#include "support/myallocator.h"

#define DEFAULT_NUM_TOTAL_OBJS 1000000

class manager_base
{
public:
  virtual ~manager_base() = default;
  virtual size_t get_size() const = 0;
};

template <typename T>
class manager : public manager_base //hash-consing implementation
{
public:
  manager()
  {
    int status;
    char * demangled = abi::__cxa_demangle(typeid(T).name(),0,0,&status);
    stats::get().register_manager(demangled, *this);
    free(demangled);
  }
  ~manager()
  {
    int status;
    char * demangled = abi::__cxa_demangle(typeid(T).name(),0,0,&status);
    stats::get().deregister_manager(demangled);
    free(demangled);
  }
  std::shared_ptr<T> register_object(T const &given, unsigned suggested_id = 0)
  {
    //stopwatch_run(&manager_register_object_timer);
    auto it = m_object_map.find(given);
    if (it == m_object_map.end()) {
      //std::shared_ptr<T> new_elem = std::make_shared<T>(given);
      std::shared_ptr<T> new_elem = std::allocate_shared<T>(m_myallocator, given);
      new_elem->set_id_to_free_id(suggested_id);
      assert(m_object_map.insert(make_pair(given, new_elem)).second);
      //std::cout << __func__ << " " << __LINE__ << ": returning " << new_elem.get() << " for " << typeid(T).name() << std::endl;
      //stopwatch_stop(&manager_register_object_timer);
      return new_elem;
    } else {
      std::weak_ptr<T> const &existing = it->second;
      std::shared_ptr<T> ret = existing.lock();
      assert(ret);
      //stopwatch_stop(&manager_register_object_timer);
      return ret;
    }
  }

  void deregister_object(T const &given)
  {
    //stopwatch_run(&manager_deregister_object_timer);
    assert(!given.id_is_zero());
    //std::cout << __func__ << " " << __LINE__ << ": " << typeid(T).name() << " deregistering object " << &given << std::endl;
    auto n = m_object_map.erase(given);
    assert(n == 1);
    //stopwatch_stop(&manager_deregister_object_timer);
  }

  virtual size_t get_size() const override
  {
    return m_object_map.size();
  }

  std::string stat() const
  {
    std::stringstream ss;
    ss << "count: " << m_object_map.size();
    return ss.str();
  }

private:
  std::unordered_map<T, std::weak_ptr<T>> m_object_map;
  myallocator_t<T> m_myallocator;
};
