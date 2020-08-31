#pragma once
#include <memory>
#include <string>
#include "support/debug.h"
template <typename T> class manager; // fwd decl

using namespace std;

class string_obj_t
{
private:
  string m_str;

  bool m_is_managed;
public:
  ~string_obj_t();
  string const &get_str() const
  { return m_str; }
  bool operator==(string_obj_t const &other) const { return m_str == other.m_str; }

  bool id_is_zero() const { return !m_is_managed; }
  void set_id_to_free_id(unsigned suggested_id)
  {
    ASSERT(!m_is_managed);
    m_is_managed = true;
  }
  static manager<string_obj_t> *get_string_obj_manager();
  friend shared_ptr<string_obj_t const> mk_string_ref(string const &s);
private:
  string_obj_t() : m_is_managed(false)
  { }
  string_obj_t(string const &str) : m_str(str), m_is_managed(false)
  { }
};

using string_ref = shared_ptr<string_obj_t const>;
string_ref mk_string_ref(string const &s);

namespace std
{
template<>
struct hash<string_obj_t>
{
  std::size_t operator()(string_obj_t const &s) const;
};
}
