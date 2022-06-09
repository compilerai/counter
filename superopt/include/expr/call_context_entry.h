#pragma once

#include "support/string_ref.h"

#include "expr/pc.h"

template<typename T> class manager; // fwd decl


namespace eqspace {

class call_context_entry_t {
private:
  string_ref m_fname;
  pc m_pc;

  bool m_is_managed = false;
public:
  ~call_context_entry_t();
  string_ref const& get_fname() const { return m_fname; }
  pc const& get_pc() const { return m_pc; }

  string call_context_entry_to_string() const;
  bool operator==(call_context_entry_t const &other) const
  {
    return    m_fname == other.m_fname
           && m_pc == other.m_pc
    ;
  }
  bool id_is_zero() const { return !m_is_managed; }
  void set_id_to_free_id(unsigned suggested_id)
  {
    assert(!m_is_managed);
    m_is_managed = true;
  }
  static manager<call_context_entry_t> *get_call_context_entry_manager();

  friend shared_ptr<call_context_entry_t const> mk_call_context_entry(string_ref const &fname, pc const& p);
private:
  call_context_entry_t(string_ref const& fname, pc const& p) : m_fname(fname), m_pc(p)
  { }
};

using call_context_entry_ref = shared_ptr<call_context_entry_t const>;
call_context_entry_ref mk_call_context_entry(string_ref const &fname, pc const& p);

}

namespace std
{
template<>
struct hash<call_context_entry_t>
{
  std::size_t operator()(call_context_entry_t const &s) const;
};
}
