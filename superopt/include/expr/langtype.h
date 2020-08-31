#pragma once
#include <string>
#include <memory>
#include <vector>
#include <list>
using std::string;
#include "support/consts.h"
#include "support/manager.h"
#include "support/utils.h"
#include "cutils/chash.h"
#include "support/debug.h"
#include "expr/defs.h"

namespace eqspace {

class langtype_t;
using langtype_ref = shared_ptr<langtype_t const>;

class langtype_t {
private:
  using langtype_sort_t = enum { LANGTYPE_SORT_CHAR, LANGTYPE_SORT_SHORT, LANGTYPE_SORT_INT, LANGTYPE_SORT_LONG, LANGTYPE_SORT_LONGLONG, LANGTYPE_SORT_POINTER, LANGTYPE_SORT_ARRAY, LANGTYPE_SORT_STRUCT };
  using langtype_signedness_t = enum {LANGTYPE_SIGNED, LANGTYPE_UNSIGNED, LANGTYPE_NOSIGN};
private:
  langtype_t(string const& keyword) : m_is_managed(false)
  {
    this->from_string(keyword);
  }
public:
  ~langtype_t();
  friend shared_ptr<langtype_t const> mk_langtype_ref(string const& keyword);

  static manager<langtype_t> *get_langtype_manager();

  bool id_is_zero() const { return !m_is_managed; }
  void set_id_to_free_id(unsigned suggested_id)
  {
    ASSERT(!m_is_managed);
    m_is_managed = true;
  }

  bool operator==(langtype_t const &other) const;
  bool operator!=(langtype_t const &other) const
  {
    return !(*this == other);
  }
  bool is_character_type() const
  {
    return m_sort == LANGTYPE_SORT_CHAR;
  }
  string to_string() const;
private:
  void from_string(string const &s);
private:
  langtype_sort_t m_sort;
  langtype_signedness_t m_signedness;
  list<langtype_ref> m_referenced_types;
  size_t          m_num_elems; //for array types
  string          m_val; //for debugging

  bool            m_is_managed;
};

langtype_ref mk_langtype_ref(string const &k);

}

namespace std
{
using namespace eqspace;
template<>
struct hash<langtype_t>
{
  std::size_t operator()(eqspace::langtype_t const& l) const
  {
    std::size_t seed = 0;
    return myhash_string(l.to_string().c_str());
  }
};
}
