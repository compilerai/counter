#pragma once
#include <map>
#include <string>

#include "support/types.h"
#include "support/dyn_debug.h"
#include "graph/dfa.h"
#include "expr/sprel_map_pair.h"
//#include "graph/pc.h"
//#include "graph/itfg_edge.h"
//#include "graph/tfg_edge.h"

using namespace std;

namespace eqspace {

typedef set<string_ref> livevars_t;

template<typename T_PC, typename T_N, typename T_E, typename T_PRED>
class livevars_val_t
{
private:
  livevars_t m_live;
public:
  livevars_val_t() : m_live() { }
  livevars_val_t(livevars_t const &l) : m_live(l) { }

  static livevars_val_t top() { return livevars_val_t(); }
  
  livevars_t const &get_livevars() const { return m_live; }
  bool meet(livevars_val_t const& b)
  {
    //autostop_timer func_timer(__func__);
    //livevars_t oldval = m_live;
    size_t old_size = m_live.size();
    set_union(m_live, b.m_live);
    return m_live.size() != old_size;
  }

  static std::function<livevars_t (T_PC const &)>
  get_boundary_value(dshared_ptr<graph<T_PC, T_N, T_E> const> g)
  {
    auto f = [g](T_PC const &p)
    {
      // init_loc_liveness_to_retlive
      ASSERT(p.is_exit());

      dshared_ptr<graph_with_predicates<T_PC, T_N, T_E, T_PRED> const> gp = dynamic_pointer_cast<graph_with_predicates<T_PC, T_N, T_E, T_PRED> const>(g);
      ASSERT(gp);

      livevars_t retval;
      map<string_ref, expr_ref> const &ret_regs = gp->get_return_regs_at_pc(p);
      DYN_DEBUG(compute_var_liveness, cout << "var_liveness_dfa::get_boundary_value:" <<  __LINE__ << ": " << p.to_string() << ": ret_regs.size() = " << ret_regs.size() << endl);
      context* ctx = gp->get_context();
      for (const auto &pret : ret_regs) {
        string_ref const &ret = pret.first;
        DYN_DEBUG(compute_var_liveness, cout << "liveness_dfa::get_boundary_value:" << __LINE__ << ": " << p.to_string() << ": ret = " << ret->get_str() << ": " << expr_string(pret.second) << endl);
        list<expr_ref> const& read_var_exprs = ctx->expr_get_vars(pret.second);
        livevars_t read_vars;
        for(auto const &v : read_var_exprs) {
          DYN_DEBUG(compute_var_liveness, cout << "liveness_dfa::get_boundary_value:" << __LINE__ << ": " << p.to_string() << ": ret_expr = " << expr_string(pret.second) << ", read_var = " << expr_string(v) << endl);
          ASSERT(v->is_var());
          if(!ctx->expr_is_input_expr(v)) 
            continue;
          string_ref vname = ctx->get_key_from_input_expr(v);
          read_vars.insert(vname);
        }
        set_union(retval, read_vars);
      }
      return retval;
    };
    return f;
  }
};

template<typename T_PC, typename T_N, typename T_E, typename T_PRED>
class var_liveness_analysis : public data_flow_analysis<T_PC, T_N, T_E, livevars_val_t<T_PC, T_N, T_E, T_PRED>, dfa_dir_t::backward>
{
public:

  var_liveness_analysis(dshared_ptr<graph_with_regions_t<T_PC, T_N, T_E> const> g, map<T_PC, livevars_val_t<T_PC, T_N, T_E, T_PRED>> &init_vals)
    : data_flow_analysis<T_PC, T_N, T_E, livevars_val_t<T_PC, T_N, T_E, T_PRED>, dfa_dir_t::backward>(g, init_vals)
  {
    //ASSERT((dynamic_cast<graph_with_simplified_assets<T_PC, T_N, T_E, T_PRED> const*>(g) != nullptr));
  }

  virtual dfa_xfer_retval_t xfer_and_meet(dshared_ptr<T_E const> const &e, livevars_val_t<T_PC, T_N, T_E, T_PRED> const& in, livevars_val_t<T_PC, T_N, T_E, T_PRED> &out) override
  {
    autostop_timer func_timer(__func__);
    livevars_t retval;

    DYN_DEBUG(compute_var_liveness, cout << __func__ << ':' << __LINE__ << ": " << e->to_string() << " liveness in: "; for (auto const& l : in.get_livevars()) cout << l->get_str() << ", "; cout << endl);
//    map<string_ref, expr_ref> const &vars_modified = gp->graph_get_vars_written_on_edge(e);
//    DYN_DEBUG(compute_var_liveness, cout << __func__ << ':' << __LINE__ << ": vars written on edge:";
//      for (auto const& ve : vars_modified) {
//        cout << " " << ve.first->get_str();
//      }
//      cout << endl;
//    );
    state const& e_state = e->get_to_state();
    set<expr_ref> live_exprs;
    for (const auto &v : in.get_livevars()) {
      string v_name = v->get_str(); //.substr(strlen(G_INPUT_KEYWORD "."));
      if(e_state.key_exists(v_name)) {// v is written on this edge
        DYN_DEBUG(compute_var_liveness, cout << _FNLN_ << ": modified var = " << v->get_str() << endl);
        expr_ref const &v_expr = e_state.get_expr(v_name);
        live_exprs.insert(v_expr);
      } else {
        retval.insert(v);
      }
    }
    expr_ref const &econd = e->get_cond();
    context* ctx = econd->get_context();
    live_exprs.insert(econd);
    for(auto const& l : live_exprs) {
      list<expr_ref> const& read_vars = ctx->expr_get_vars(l);
      set<string_ref> read_vars_str;
      for(auto const& v : read_vars) {
        DYN_DEBUG(compute_var_liveness, cout << "liveness_dfa::xfer_and_meet:" << __LINE__ << ": live_expr = " << expr_string(l) << ", read_var = " << expr_string(v) << endl);
        if(!ctx->expr_is_input_expr(v)) 
          continue;
        string_ref vname = ctx->get_key_from_input_expr(v);
        read_vars_str.insert(vname);
      }
      DYN_DEBUG(compute_var_liveness, cout << _FNLN_ << ": read vars = "; for (auto const& v : read_vars_str) cout << v->get_str() << ", "; cout << endl);
      set_union(retval, read_vars_str);
    }

    DYN_DEBUG(compute_var_liveness, cout << __func__ << ':' << __LINE__ << ": " << e->to_string_concise() << "\n liveness out: "; for (auto const& l : retval) cout << l->get_str() << ", "; cout << endl);
    return out.meet(retval) ? DFA_XFER_RETVAL_CHANGED : DFA_XFER_RETVAL_UNCHANGED;
  }
};

}
