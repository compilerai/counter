#pragma once

#include "expr/call_context.h"

namespace eqspace {

template <typename T_VAL>
class context_sensitive_ftmap_dfa_val_t
{
private:
  mutable map<call_context_ref, T_VAL> m_vals;
public:
  context_sensitive_ftmap_dfa_val_t(call_context_ref const& cc, T_VAL const& val)
  {
    bool inserted = m_vals.insert(make_pair(cc, val)).second;
    ASSERT(inserted);
  }
  //T_VAL& get_val() { return m_val; }
  //T_VAL const& get_val() const { return m_val; }
  map<call_context_ref, T_VAL> const& get_vals() const { return m_vals; }

  T_VAL const& get_val_at_call_context(call_context_ref const& cc) const
  {
    ASSERT(m_vals.count(cc));
    //if (!m_vals.count(cc)) {
    //  m_vals.insert(make_pair(cc, T_VAL::top()));
    //}
    return m_vals.at(cc);
  }

  T_VAL& get_val_at_call_context(call_context_ref const& cc)
  {
    ASSERT(m_vals.count(cc));
    //if (!m_vals.count(cc)) {
    //  m_vals.insert(make_pair(cc, T_VAL::top()));
    //}
    return m_vals.at(cc);
  }

  void context_sensitive_ftmap_dfa_val_add_mapping(call_context_ref const& cc, T_VAL const& val) const
  {
    bool inserted = m_vals.insert(make_pair(cc, val)).second;
    ASSERT(inserted);
  }

  static context_sensitive_ftmap_dfa_val_t<T_VAL> top()
  {
    return context_sensitive_ftmap_dfa_val_t<T_VAL>();
  }

  context_sensitive_ftmap_dfa_val_t()
  { }
};

}
