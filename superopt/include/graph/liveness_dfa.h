#pragma once

#include "support/types.h"
#include "support/dyn_debug.h"
#include "graph/dfa.h"
#include "expr/sprel_map_pair.h"
#include "graph/graph_with_locs.h"
#include "graph/graph_with_simplified_assets.h"
//#include "graph/pc.h"
//#include "graph/itfg_edge.h"
//#include "graph/tfg_edge.h"

using namespace std;

namespace eqspace {

typedef set<graph_loc_id_t> livelocs_t;

template<typename T_PC, typename T_N, typename T_E, typename T_PRED>
class livelocs_val_t
{
private:
  livelocs_t m_live;
public:
  livelocs_val_t() : m_live() { }
  livelocs_val_t(livelocs_t const &l) : m_live(l) { }

  static livelocs_val_t top() { return livelocs_val_t(); }
  
  livelocs_t const &get_livelocs() const { return m_live; }
  bool meet(livelocs_val_t const& b)
  {
    //autostop_timer func_timer(__func__);
    //livelocs_t oldval = m_live;
    size_t old_size = m_live.size();
    set_union(m_live, b.m_live);
    return m_live.size() != old_size;
  }

  static std::function<livelocs_t (T_PC const &)>
  get_boundary_value(dshared_ptr<graph<T_PC, T_N, T_E> const> g)
  {
    auto f = [g](T_PC const &p)
    {
      ASSERT(p.is_exit());

      dshared_ptr<graph_with_simplified_assets<T_PC, T_N, T_E, T_PRED> const> gp = dynamic_pointer_cast<graph_with_simplified_assets<T_PC, T_N, T_E, T_PRED> const>(g);
      ASSERT(gp);

      auto ret = gp->graph_get_live_locids_at_boundary_pc(p);
      DYN_DEBUG(compute_liveness, cout << p.to_string() << ": ret.size() = " << ret.size() << ".  locids :";
                                  for (auto const& locid : ret) cout << ' ' << locid; cout << endl);
      return ret;
    };
    return f;
  }
};

template<typename T_PC, typename T_N, typename T_E, typename T_PRED>
class liveness_analysis : public data_flow_analysis<T_PC, T_N, T_E, livelocs_val_t<T_PC, T_N, T_E, T_PRED>, dfa_dir_t::backward>
{
public:

  liveness_analysis(dshared_ptr<graph_with_regions_t<T_PC, T_N, T_E> const> g, map<T_PC, livelocs_val_t<T_PC, T_N, T_E, T_PRED>> &init_vals)
    : data_flow_analysis<T_PC, T_N, T_E, livelocs_val_t<T_PC, T_N, T_E, T_PRED>, dfa_dir_t::backward>(g, init_vals)
  {
    //ASSERT((dynamic_cast<graph_with_simplified_assets<T_PC, T_N, T_E, T_PRED> const*>(g) != nullptr));
  }

  virtual dfa_xfer_retval_t xfer_and_meet(dshared_ptr<T_E const> const &e, livelocs_val_t<T_PC, T_N, T_E, T_PRED> const& in, livelocs_val_t<T_PC, T_N, T_E, T_PRED> &out) override
  {
    autostop_timer func_timer(__func__);
    dshared_ptr<graph_with_simplified_assets<T_PC,T_N,T_E,T_PRED> const> gp = dynamic_pointer_cast<graph_with_simplified_assets<T_PC,T_N,T_E,T_PRED> const>(this->m_graph);
    ASSERT(gp);
    livelocs_t retval;

    DYN_DEBUG(compute_liveness, cout << __func__ << ':' << __LINE__ << ": " << e->get_from_pc().to_string() << "=>" << e->get_to_pc().to_string() << " liveness in: "; for (auto const& l : in.get_livelocs()) cout << l << ' '; cout << endl);
    map<graph_loc_id_t, expr_ref> const &locs_modified = gp->get_locs_potentially_written_on_edge(e);
    DYN_DEBUG(compute_liveness, cout << __func__ << ':' << __LINE__ << ": locs potentially written on edge:";
      for (auto const& le : locs_modified) {
        cout << " " << le.first;
      }
      cout << endl;
    );
    for (const auto &loc_id : in.get_livelocs()) {
      if (locs_modified.count(loc_id)) {
        expr_ref const &loc_e = locs_modified.at(loc_id);
        DYN_DEBUG(compute_liveness, cout << _FNLN_ << ": " << e->get_from_pc().to_string() << "=>" << e->get_to_pc().to_string() << ": modified " << loc_id << " = " << expr_string(loc_e) << endl);
        set<graph_loc_id_t> read_locs = gp->get_locs_potentially_read_in_expr(loc_e);
        DYN_DEBUG(compute_liveness, cout << _FNLN_ << ": " << e->get_from_pc().to_string() << "=>" << e->get_to_pc().to_string() << ": read locs = "; for (auto const& l : read_locs) cout << l << ' '; cout << endl);
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

    DYN_DEBUG(compute_liveness, cout << __func__ << ':' << __LINE__ << ": " << e->to_string_concise() << "\n liveness out: "; for (auto const& l : retval) cout << l << ' '; cout << endl);
    return out.meet(retval) ? DFA_XFER_RETVAL_CHANGED : DFA_XFER_RETVAL_UNCHANGED;
  }
};

}
