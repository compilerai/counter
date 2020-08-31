#pragma once
#include <map>
#include <string>

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
    livelocs_t oldval = m_live;
    set_union(m_live, b.m_live);
    return m_live != oldval;
  }

  static std::function<livelocs_t (T_PC const &)>
  get_boundary_value(graph<T_PC, T_N, T_E> const*g)
  {
    auto f = [g](T_PC const &p)
    {
      // init_loc_liveness_to_retlive
      ASSERT(p.is_exit());

      graph_with_simplified_assets<T_PC, T_N, T_E, T_PRED>const* gp = dynamic_cast<graph_with_simplified_assets<T_PC, T_N, T_E, T_PRED> const*>(g);

      livelocs_t retval;
      map<string_ref, expr_ref> const &ret_regs = gp->get_return_regs_at_pc(p);
      DYN_DEBUG(compute_liveness, cout << "liveness_dfa::get_boundary_value:" <<  __LINE__ << ": " << p.to_string() << ": ret_regs.size() = " << ret_regs.size() << endl);
      for (const auto &pret : ret_regs) {
        expr_ref const &ret = pret.second;

        DYN_DEBUG(compute_liveness, cout << "liveness_dfa::get_boundary_value:" << __LINE__ << ": " << p.to_string() << ": ret = " << expr_string(ret) << endl);
        set<graph_loc_id_t> read = gp->get_locs_potentially_read_in_expr(ret);
        set_union(retval, read);
      }
      return retval;
    };
    return f;
  }
};

template<typename T_PC, typename T_N, typename T_E, typename T_PRED>
class liveness_analysis : public data_flow_analysis<T_PC, T_N, T_E, livelocs_val_t<T_PC, T_N, T_E, T_PRED>, dfa_dir_t::backward>
{
public:

  liveness_analysis(graph<T_PC, T_N, T_E> const* g, map<T_PC, livelocs_val_t<T_PC, T_N, T_E, T_PRED>> &init_vals)
    : data_flow_analysis<T_PC, T_N, T_E, livelocs_val_t<T_PC, T_N, T_E, T_PRED>, dfa_dir_t::backward>(g, init_vals)
  {
    ASSERT((dynamic_cast<graph_with_simplified_assets<T_PC, T_N, T_E, T_PRED> const*>(g) != nullptr));
  }

  virtual bool xfer_and_meet(shared_ptr<T_E const> const &e, livelocs_val_t<T_PC, T_N, T_E, T_PRED> const& in, livelocs_val_t<T_PC, T_N, T_E, T_PRED> &out) override
  {
    autostop_timer func_timer(__func__);
    graph_with_simplified_assets<T_PC,T_N,T_E,T_PRED>const* gp = dynamic_cast<graph_with_simplified_assets<T_PC,T_N,T_E,T_PRED> const*>(this->m_graph);
    livelocs_t retval;

    DYN_DEBUG(compute_liveness, cout << __func__ << ':' << __LINE__ << ": " << e->to_string() << " liveness in: "; for (auto const& l : in.get_livelocs()) cout << l << ' '; cout << endl);
    map<graph_loc_id_t, expr_ref> const &locs_modified = gp->get_locs_potentially_written_on_edge(e);
    for (const auto &loc_id : in.get_livelocs()) {
      if (locs_modified.count(loc_id)) {
        expr_ref const &loc_e = locs_modified.at(loc_id);
        DYN_DEBUG(compute_liveness, cout << _FNLN_ << ": modified " << loc_id << " = " << expr_string(loc_e) << endl);
        set<graph_loc_id_t> read_locs = gp->get_locs_potentially_read_in_expr(loc_e);
        DYN_DEBUG(compute_liveness, cout << _FNLN_ << ": read locs = "; for (auto const& l : read_locs) cout << l << ' '; cout << endl);
        set_union(retval, read_locs);
      } else {
        retval.insert(loc_id);
      }
    }
    expr_ref const &econd = e->get_simplified_cond();
    set<graph_loc_id_t> read_locs = gp->get_locs_potentially_read_in_expr(econd);
    set_union(retval, read_locs);
    expr_ref const &econd_unsimplified = e->get_cond();
    sprel_map_pair_t sprel_map_pair;
    expr_ref memlabel_simplified_econd = gp->get_context()->expr_simplify_using_sprel_pair_and_memlabel_maps(econd_unsimplified, sprel_map_pair_t(), gp->get_memlabel_map());
    read_locs = gp->get_locs_potentially_read_in_expr(memlabel_simplified_econd);
    set_union(retval, read_locs);

    DYN_DEBUG(compute_liveness, cout << __func__ << ':' << __LINE__ << ": " << e->to_string() << " liveness out: "; for (auto const& l : retval) cout << l << ' '; cout << endl);
    return out.meet(retval);
  }
};

}
