#pragma once
#include <map>
#include <string>

#include "support/types.h"

#include "expr/sprel_map_pair.h"
#include "expr/pc.h"

#include "gsupport/tfg_edge.h"

#include "graph/dfa.h"
#include "graph/graph_with_simplified_assets.h"

using namespace std;

namespace eqspace {

typedef set<graph_loc_id_t> bavlocs_t;

class bavlocs_val_t
{
private:
  bavlocs_t m_bav;
  bool m_is_top;
public:
  bavlocs_val_t() : m_bav(), m_is_top(false) { }
  bavlocs_val_t(bavlocs_t const &l) : m_bav(l), m_is_top(false) { }

  static bavlocs_val_t top()
  {
    bavlocs_val_t ret;
    ret.m_is_top = true;
    return ret;
  }
  
  bavlocs_t const &get_bavlocs() const { return m_bav; }
  bool meet(bavlocs_val_t const& b)
  {
    //autostop_timer func_timer(string("bavlocs_val_t.")+__func__);
    ASSERT(!b.m_is_top);
    if (this->m_is_top) {
      *this = b;
      return true;
    }
    size_t oldval_size = m_bav.size();
    set_union(m_bav, b.m_bav);
    return m_bav.size() != oldval_size;
  }

  static std::function<bavlocs_t (pc const &)>
  get_boundary_value(dshared_ptr<graph<pc, tfg_node, tfg_edge> const> g)
  {
    auto f = [](pc const &p)
    {
      // empty set
      return bavlocs_t();
    };
    return f;
  }
};

class branch_affecting_vars_analysis : public data_flow_analysis<pc, tfg_node, tfg_edge, bavlocs_val_t, dfa_dir_t::backward>
{
public:

  branch_affecting_vars_analysis(dshared_ptr<graph_with_regions_t<pc, tfg_node, tfg_edge> const> g, map<pc, bavlocs_val_t> &init_vals)
    : data_flow_analysis<pc, tfg_node, tfg_edge, bavlocs_val_t, dfa_dir_t::backward>(g, init_vals)
  {
    //ASSERT((dynamic_cast<graph_with_simplified_assets<pc, tfg_node, tfg_edge, predicate> const*>(g) != nullptr));
  }

  virtual ~branch_affecting_vars_analysis() = default;

  virtual dfa_xfer_retval_t xfer_and_meet(dshared_ptr<tfg_edge const> const &e, bavlocs_val_t const& in, bavlocs_val_t &out) override
  {
    //autostop_timer func_timer(string("branch_affecting_vars_analysis.")+__func__);
    dshared_ptr<graph_with_simplified_assets<pc, tfg_node, tfg_edge, predicate> const> gp = dynamic_pointer_cast<graph_with_simplified_assets<pc, tfg_node, tfg_edge, predicate> const>(this->m_graph);
    bavlocs_t retval;

    //cout << __func__ << ':' << __LINE__ << ": " << e->to_string() << " bav in: "; for (auto const& l : in.get_bavlocs()) cout << l << ' '; cout << endl;
    map<graph_loc_id_t, expr_ref> const &locs_modified = gp->get_locs_potentially_written_on_edge(e);
    for (const auto &loc_id : in.get_bavlocs()) {
      if (locs_modified.count(loc_id)) {
        expr_ref const &e = locs_modified.at(loc_id);
        set<graph_loc_id_t> read_locs = gp->get_locs_potentially_read_in_expr(e);
        //cout << __func__ << ':' << __LINE__ << ": modified " << loc_id << " = " << expr_string(e) << " ; read locs = "; for (auto const& l : read_locs) cout << l << ' '; cout << endl;
        set_union(retval, read_locs);
      } else {
        retval.insert(loc_id);
      }
    }
    //expr_ref const &econd = e->get_simplified_cond();
    expr_ref const &econd = gp->get_simplified_edge_cond_for_edge(e);
    set<graph_loc_id_t> read_locs = gp->get_locs_potentially_read_in_expr(econd);
    set_union(retval, read_locs);
    expr_ref const &econd_unsimplified = e->get_cond();
    sprel_map_pair_t sprel_map_pair;
    expr_ref memlabel_simplified_econd = gp->get_context()->expr_simplify_using_sprel_pair_and_memlabel_maps(econd_unsimplified, sprel_map_pair_t(), gp->get_memlabel_map(call_context_t::call_context_null()));
    read_locs = gp->get_locs_potentially_read_in_expr(memlabel_simplified_econd);
    set_union(retval, read_locs);

    //cout << __func__ << ':' << __LINE__ << ": " << e->to_string() << " bav out: "; for (auto const& l : retval) cout << l << ' '; cout << endl;
    return out.meet(retval) ? DFA_XFER_RETVAL_CHANGED : DFA_XFER_RETVAL_UNCHANGED;
  }
};

}
