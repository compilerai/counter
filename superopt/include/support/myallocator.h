#ifndef MYALLOCATOR_H
#define MYALLOCATOR_H

#include <cstdlib>
#include <new>
#include <limits>
#include <iostream>
#include <vector>

#include "support/globals_cpp.h"
#include "support/debug.h"

extern "C" {
void *deterministic_malloc(size_t);
void deterministic_free(void*);
}

//https://en.cppreference.com/w/cpp/named_req/Allocator
template <class T>
struct myallocator_t
{
  typedef T value_type;
 
  myallocator_t () = default;
  template <class U> constexpr myallocator_t (const myallocator_t <U>&) noexcept {}
 
  [[nodiscard]] T* allocate(std::size_t n) {
    if (n > std::numeric_limits<std::size_t>::max() / sizeof(T))
      throw std::bad_alloc();
 
    ASSERT(!g_expect_no_deterministic_malloc_calls);
    if (auto p = static_cast<T*>(deterministic_malloc(n*sizeof(T)))) {
      static string prefix = string("DMALLOC.") + typeid(T).name();
      if (g_scan_alloc_ids_fd != -1) {
        scan_alloc_id(prefix.c_str(), (unsigned)n, p);
      }
      if (g_print_alloc_ids_fd != -1) {
        print_alloc_id(prefix.c_str(), (unsigned)n, p);
      }

      //report(p, n);
      return p;
    }
 
    throw std::bad_alloc();
  }
 
  void deallocate(T* p, std::size_t n) noexcept {
    //report(p, n, 0);
    static string prefix = string("DFREE.") + typeid(T).name();
    if (g_scan_alloc_ids_fd != -1) {
      scan_alloc_id(prefix.c_str(), (unsigned)n, p);
    }
    if (g_print_alloc_ids_fd != -1) {
      print_alloc_id(prefix.c_str(), (unsigned)n, p);
    }
    ASSERT(!g_expect_no_deterministic_malloc_calls);
    deterministic_free(p);
  }
 
private:
  void report(T* p, std::size_t n, bool alloc = true) const {
    std::cout << (alloc ? "Alloc: " : "Dealloc: ") << sizeof(T)*n
      << " bytes at " << std::hex << std::showbase
      << reinterpret_cast<void*>(p) << std::dec << '\n';
  }
};
 
template <class T, class U>
bool operator==(const myallocator_t <T>&, const myallocator_t <U>&) { return true; }

template <class T, class U>
bool operator!=(const myallocator_t <T>&, const myallocator_t <U>&) { return false; }

#endif
