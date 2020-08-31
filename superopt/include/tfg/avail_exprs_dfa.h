#pragma once
#include <map>
#include <string>

#include "expr/expr.h"
#include "support/types.h"
#include "gsupport/pc.h"
#include "gsupport/tfg_edge.h"
#include "graph/dfa.h"
#include "tfg/avail_exprs.h"

using namespace std;

namespace eqspace {

class avail_exprs_val_t
{
private:
  avail_exprs_t m_exprs;
  bool m_is_top;
public:
  avail_exprs_val_t() : m_is_top(false) { }
  avail_exprs_val_t(avail_exprs_t const &exprs) : m_exprs(exprs), m_is_top(false) { }
  static avail_exprs_val_t
  top()
  {
    avail_exprs_val_t ret;
    ret.m_is_top = true;
    return ret;
  }
  avail_exprs_t const &get_avail() const { /*ASSERT(!m_is_top);*/ return m_exprs; }
  bool meet(avail_exprs_val_t const &other)
  {
    ASSERT(!other.m_is_top);
    if (this->m_is_top) {
      *this = other;
      return true;
    }
    autostop_timer func_timer(string("avail_exprs_analysis.") + __func__);
    // convert map to set for set intersection
    set<pair<graph_loc_id_t, expr_ref>> set_a;
    set<pair<graph_loc_id_t, expr_ref>> set_b;

    for (auto const& p : m_exprs)
      set_a.insert(p);
    for (auto const& p : other.m_exprs)
      set_b.insert(p);

    set_intersect(set_a, set_b);

    // set to map
    auto new_val = avail_exprs_t(set_a.begin(), set_a.end());
    if (new_val != m_exprs) {
      m_exprs = new_val;
      return true;
    }
    return false;
  }
};

class avail_exprs_analysis : public data_flow_analysis<pc, tfg_node, tfg_edge, avail_exprs_val_t, dfa_dir_t::forward>
{
public:

  avail_exprs_analysis(graph<pc, tfg_node, tfg_edge> const* g, map<pc, avail_exprs_val_t> &init_vals);

  virtual bool xfer_and_meet(shared_ptr<tfg_edge const> const &e, avail_exprs_val_t const& in, avail_exprs_val_t &out) override;
};

}
