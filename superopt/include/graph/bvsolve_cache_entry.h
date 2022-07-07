#pragma once

#include "support/types.h"
#include "support/bv_const.h"

#include "graph/expr_group.h"

using namespace std;

namespace eqspace {

class bvsolve_cache_entry_t
{
private:
  expr_group_ref m_exprs = nullptr;
  set<expr_group_t::expr_idx_t> m_out_of_bound_exprs = {};
  set<string_ref> m_ce_names = {};
  bvsolve_retval_t m_bvsolve_retval;
public:
  bvsolve_cache_entry_t(expr_group_ref const& exprs,
                        set<expr_group_t::expr_idx_t> const& out_of_bound_exprs,
                        set<string_ref> const& ce_names,
                        bvsolve_retval_t const& bvsolve_retval)
    : m_exprs(exprs),
      m_out_of_bound_exprs(out_of_bound_exprs),
      m_ce_names(ce_names),
      m_bvsolve_retval(bvsolve_retval)
  { }

  expr_group_ref const& get_exprs() const { return m_exprs; }
  set<string_ref> const& get_ce_names() const { return m_ce_names; }
  set<expr_group_t::expr_idx_t> const& get_out_of_bound_exprs() const { return m_out_of_bound_exprs; }
  bvsolve_retval_t const& get_bvsolve_retval() const { return m_bvsolve_retval; }
};

}
