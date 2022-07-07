#pragma once

#include "graph/dfa_types.h"

namespace eqspace {

using namespace std;

template <typename T_PC, bool USE_UNION_FOR_MEET>
class pc_list_val_t {
private:
  list<T_PC> m_pc_list;
  bool m_is_top;
public:
  static pc_list_val_t<T_PC, USE_UNION_FOR_MEET> singleton(T_PC const& p)
  {
    pc_list_val_t<T_PC, USE_UNION_FOR_MEET> ret;
    ret.m_pc_list.push_back(p);
    ASSERT(!ret.m_is_top);
    return ret;
  }

  static pc_list_val_t<T_PC, USE_UNION_FOR_MEET> top()
  {
    pc_list_val_t<T_PC, USE_UNION_FOR_MEET> ret;
    ret.m_is_top = true;
    return ret;
  }

  bool is_top() const { return m_is_top; }
  list<T_PC> const& get_pc_list() const { return m_pc_list; }
  pc_list_val_t<T_PC, USE_UNION_FOR_MEET> add(T_PC const& p) const
  {
    ASSERT(!m_is_top);
    pc_list_val_t ret = *this;
    ret.m_pc_list.push_back(p);
    return ret;
  }
  pc_list_val_t<T_PC, USE_UNION_FOR_MEET> pop_back() const
  {
    ASSERT(!m_is_top);
    pc_list_val_t ret = *this;
    ret.m_pc_list.pop_back();
    return ret;
  }


  dfa_xfer_retval_t meet(pc_list_val_t<T_PC, USE_UNION_FOR_MEET> const& other)
  {
    ASSERT(!other.m_is_top);
    if (this->m_is_top) {
      *this = other;
      return DFA_XFER_RETVAL_CHANGED;
    }
    auto orig_size = this->m_pc_list.size();
    if (USE_UNION_FOR_MEET) {
      list_union_append(this->m_pc_list, other.m_pc_list);
    } else {
      list_intersect(this->m_pc_list, other.m_pc_list);
    }
    return (orig_size != this->m_pc_list.size()) ? DFA_XFER_RETVAL_CHANGED : DFA_XFER_RETVAL_UNCHANGED;
  }
private:
  pc_list_val_t() : m_is_top(false) { }
};

template<typename T_PC>
using dominator_val_t = pc_list_val_t<T_PC, false>;


template<typename T_PC>
using reaching_pcs_val_t = pc_list_val_t<T_PC, true>;
}
