#pragma once
#include <map>
#include <string>

#include "support/types.h"

#include "expr/expr.h"
#include "expr/pc.h"

#include "gsupport/tfg_edge.h"

#include "graph/dfa.h"
#include "graph/avail_exprs.h"

using namespace std;

namespace eqspace {

class avail_exprs_dfa_val_t
{
private:
  avail_exprs_t m_exprs;
  bool m_is_top;
public:
  avail_exprs_dfa_val_t() : m_is_top(false) { }
  avail_exprs_dfa_val_t(avail_exprs_t const &exprs) : m_exprs(exprs), m_is_top(false) { }
  static avail_exprs_dfa_val_t
  top()
  {
    avail_exprs_dfa_val_t ret;
    ret.m_is_top = true;
    return ret;
  }

  avail_exprs_t const &get_avail() const { /*ASSERT(!m_is_top);*/ return m_exprs; }
  friend class avail_exprs_analysis<pc, tfg_node, tfg_edge, predicate>;
  friend class avail_exprs_analysis<pcpair, corr_graph_node, corr_graph_edge, predicate>;
  //bool meet(avail_exprs_val_t const &other);
};

template<typename T_PC, typename T_N, typename T_E, typename T_PRED>
class avail_exprs_analysis : public data_flow_analysis<T_PC, T_N, T_E, avail_exprs_dfa_val_t, dfa_dir_t::forward>
{
public:

  avail_exprs_analysis(dshared_ptr<graph_with_regions_t<T_PC, T_N, T_E> const> g, map<T_PC, avail_exprs_dfa_val_t> &init_vals);

  virtual dfa_xfer_retval_t xfer_and_meet(dshared_ptr<T_E const> const &e, avail_exprs_dfa_val_t const& in, avail_exprs_dfa_val_t &out) override;
private:
  expr_ref expr_substitute_using_available_exprs(expr_ref const& e, avail_exprs_t const& a) const;
//  bool expr_contains_only_ssa_vars(expr_ref const& e) const;
  bool expr_is_ssa_var(expr_ref const& e) const;
  bool meet(avail_exprs_dfa_val_t& this_, avail_exprs_dfa_val_t const& other) const;
};

}
