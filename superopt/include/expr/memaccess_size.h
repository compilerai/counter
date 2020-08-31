#pragma once

namespace eqspace {

class memaccess_size_t {
private:
  static size_t const memaccess_size_unknown = SIZE_MAX;
public:
  memaccess_size_t(size_t sz) : m_size(sz)
  { }
  static memaccess_size_t unknown() { return memaccess_size_t(memaccess_size_unknown); }
  size_t get() const { ASSERT(m_size != memaccess_size_unknown); return m_size; }
  size_t is_unknown() const { return m_size == memaccess_size_unknown; }
private:
  size_t m_size;
};

using memaccess_type_t = enum { MEMACCESS_TYPE_LOAD = 0, MEMACCESS_TYPE_STORE };

}
