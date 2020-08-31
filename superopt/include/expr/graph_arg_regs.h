#pragma once

#include <map>
#include <string>

#include "support/types.h"
#include "expr/expr.h"

namespace eqspace {

using namespace std;

class graph_arg_regs_t {
private:
  map<string_ref, expr_ref> m_regs;
public:
  graph_arg_regs_t()
  { }
  graph_arg_regs_t(map<string_ref, expr_ref> const& regs) : m_regs(regs)
  { }
  map<string_ref, expr_ref> const& get_map() const { return m_regs; }
  size_t size() const { return m_regs.size(); }
  size_t get_num_bytes_on_stack_including_retaddr() const
  {
    size_t ret = DWORD_LEN/BYTE_LEN; //for return address
    for (const auto &a : m_regs) {
      ASSERT(a.second->is_bool_sort() || a.second->is_bv_sort());
      if (a.second->is_bool_sort()) {
        ret += DWORD_LEN/BYTE_LEN;
        //cout << __func__ << " " << __LINE__ << ": ret = " << ret << endl;
      } else {
        ret += DIV_ROUND_UP(a.second->get_sort()->get_size(), DWORD_LEN) * (DWORD_LEN/BYTE_LEN);
        //cout << __func__ << " " << __LINE__ << ": ret = " << ret << endl;
      }
    }
    return ret;
  }
  string expr_get_argname(expr_ref const &e) const
  {
    ASSERT(e->is_var());
    for (const auto &a : m_regs) {
      if (a.second == e) {
        return a.first->get_str();
      }
    }
    return "";
  }
  bool expr_is_arg(expr_ref const& e) const
  {
    return expr_get_argname(e) != "";
  }
  bool count(string_ref const& s) const { return m_regs.count(s) != 0; }
  expr_ref const& at(string_ref const& s) const { return m_regs.at(s); }
};

}
