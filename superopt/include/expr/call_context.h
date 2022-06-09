#pragma once

#include "expr/call_context_entry.h"

namespace eqspace {

class call_context_t;
using call_context_ref = shared_ptr<call_context_t const>;

class call_context_t {
private:
  list<call_context_entry_ref> m_stack;
  string_ref m_cur_function;
  bool m_has_any_prefix = false;

  bool m_is_managed = false;
public:
  ~call_context_t();
  list<call_context_entry_ref> const& get_stack() const { return m_stack; }
  string_ref call_context_get_cur_fname() const;
  bool call_context_has_any_prefix() const { return m_has_any_prefix; }

  bool operator==(call_context_t const &other) const
  {
    return     this->m_stack == other.m_stack
            && m_cur_function == other.m_cur_function
            && m_has_any_prefix == other.m_has_any_prefix
    ;
  }
  bool id_is_zero() const { return !m_is_managed; }
  void set_id_to_free_id(unsigned suggested_id)
  {
    assert(!m_is_managed);
    m_is_managed = true;
  }
  static manager<call_context_t> *get_call_context_manager();

  string call_context_to_string() const;

  bool call_context_has_suffix(call_context_ref const& other) const;
  bool call_context_is_longer_than_other(call_context_ref const& other) const;

  bool call_context_is_null() const;

  static call_context_ref call_context_append_with_bound(call_context_ref const& caller_cc, pc const& caller_pc, string const& callee_name, int max_call_context_depth);
  static call_context_ref call_context_from_string(string const& s);
  static call_context_ref empty_call_context(string const& function_name);
  static call_context_ref call_context_null();

  //friend shared_ptr<call_context_t const> mk_call_context(list<call_context_entry_ref> const& stck, string const& fname, bool use_any_prefix);

private:
  call_context_t(list<call_context_entry_ref> const& stck, string const& fname, bool use_any_prefix) : m_stack(stck), m_cur_function(mk_string_ref(fname)), m_has_any_prefix(use_any_prefix)
  { }

  static call_context_ref mk_call_context(list<call_context_entry_ref> const& stck, string const& fname, bool use_any_prefix);
};

}

namespace std
{
template<>
struct hash<call_context_t>
{
  std::size_t operator()(call_context_t const &s) const;
};
}

