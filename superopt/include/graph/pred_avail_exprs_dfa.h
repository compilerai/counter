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

class pred_avail_exprs_val_t
{
private:
  pred_avail_exprs_t m_avail;
  bool m_is_top;
public:
  pred_avail_exprs_val_t() : m_is_top(false) { }
  pred_avail_exprs_val_t(pred_avail_exprs_t const &a) : m_avail(a), m_is_top(false) { }

  static pred_avail_exprs_val_t
  top()
  {
    pred_avail_exprs_val_t ret;
    ret.m_is_top = true;
    return ret;
  }

  pred_avail_exprs_t const &get_avail() const { return m_avail; }
  pred_avail_exprs_t &get_avail_ref() { return m_avail; }

  bool meet(pred_avail_exprs_val_t const &other, tfg const *g);

  template<typename T_PC, typename T_N, typename T_E, typename T_PRED>
  static std::function<pred_avail_exprs_val_t (pc const &)>
  get_boundary_value(dshared_ptr<graph<T_PC, T_N, T_E> const> g, map<pc, tfg_suffixpath_t> const &suffixpaths, avail_exprs_t &all_uninit);
};

class pred_avail_exprs_analysis : public data_flow_analysis<pc, tfg_node, tfg_edge, pred_avail_exprs_val_t, dfa_dir_t::forward>
{
public:

  pred_avail_exprs_analysis(graph<pc, tfg_node, tfg_edge> const* g, map<pc,tfg_suffixpath_t> const &suffixpaths, map<pc, pred_avail_exprs_val_t> &init_vals);

  //pred_avail_exprs_t meet(pred_avail_exprs_t const& a, pred_avail_exprs_t const& b) override;
  //pred_avail_exprs_t xfer(shared_ptr<tfg_edge> e, pred_avail_exprs_t const& in) override;

  virtual dfa_xfer_retval_t xfer_and_meet(shared_ptr<tfg_edge const> const &e, pred_avail_exprs_val_t const& in, pred_avail_exprs_val_t &out) override;

  //static pred_avail_exprs_t copy_live_locs_from_in(pred_avail_exprs_t const &in, tfg const *t, pc const &p);
  //pred_avail_exprs_t get_boundary_value(pc const& p) override;
  bool postprocess(bool changed) override;

  static void verify_meet_invariant(pred_avail_exprs_t const& this_out, pred_avail_exprs_t const& a, pred_avail_exprs_t const& b);

  avail_exprs_t &get_all_uninit_ref() { return m_all_uninit; }

private:

  static tfg_suffixpath_t make_sf_from_parallel_sf(set<pair<tfg_suffixpath_t,expr_ref>> const& sfs, map<vector<tfg_suffixpath_t>,tfg_suffixpath_t> &make_parallel_cache);

  map<pc, tfg_suffixpath_t> m_suffixpaths;
  avail_exprs_t m_all_uninit;
};

}

