#pragma once

#include "graph/sp_version_relations_val.h"

namespace eqspace {

using sp_version_relations_t = graph_per_loc_dfa_val_t<sp_version_relations_val_t>;

set<predicate_ref> sp_version_relations_to_preds(sp_version_relations_t const& spvr, map<graph_loc_id_t,expr_ref> const& locid2expr);

class sp_version_relations_lattice_t
{
  //set<predicate_ref> m_preds;
  expr_ref m_preds_expr;
  map<memlabel_ref,tuple<bool,expr_ref,expr_ref>> m_ml_to_is_absent_lb_ub;

  bool is_less_than(expr_ref const& a, expr_ref const& b) const;
  bool addr_range_is_outside_local(expr_ref const& lb, expr_ref const& ub, memlabel_ref const& mlr) const;

  template<typename T_PC,typename T_N,typename T_E,typename T_PRED>
  static map<memlabel_ref,tuple<bool,expr_ref,expr_ref>> construct_ml_to_is_absent_lb_ub_versions_at_pc(T_PC const& pp, sprel_map_pair_t const& sprel_map_pair, graph_with_aliasing<T_PC,T_N,T_E,T_PRED> const& g)
  {
    string const& srcdst_keyword = g.get_srcdst_keyword()->get_str();
    context* ctx = g.get_context();

    map<memlabel_ref,tuple<bool,expr_ref,expr_ref>> ret;
    for (auto const& mlr : g.graph_get_non_arg_local_memlabels()) {
      memlabel_t const& ml = mlr->get_ml();
      expr_ref lb        = g.get_var_version_at_pc(pp, ctx->get_key_for_local_memlabel_lb(ml, srcdst_keyword));
      expr_ref ub        = g.get_var_version_at_pc(pp, ctx->get_key_for_local_memlabel_ub(ml, srcdst_keyword));
      expr_ref is_absent = g.get_var_version_at_pc(pp, ctx->get_key_for_local_memlabel_is_absent(ml, srcdst_keyword));

      expr_ref is_absent_simpl = ctx->expr_simplify_using_sprel_pair_and_memlabel_maps(is_absent, sprel_map_pair, graph_memlabel_map_t()); // memlabel map is not required because is-absent is assigned scalar values only
      bool is_absent_at_pc = pp.is_start() || is_absent_simpl->is_const_bool_true();

      ret.emplace(mlr,make_tuple(is_absent_at_pc,lb,ub));
    }
    return ret;
  }

  static expr_ref predicate_set_to_expr(set<predicate_ref> const& in, context* ctx)
  {
    expr_ref ret = expr_true(ctx);
    for (auto const& p : in) {
      expr_ref pe = p->predicate_get_expr();
      ret = expr_and(ret, pe);
    }
    return ret;
  }

  sp_version_relations_lattice_t(expr_ref preds_expr, map<memlabel_ref,tuple<bool,expr_ref,expr_ref>> const& ml_to_is_absent_lb_ub)
    : m_preds_expr(preds_expr), m_ml_to_is_absent_lb_ub(ml_to_is_absent_lb_ub)
  {}

public:

  template<typename T_PC,typename T_N,typename T_E,typename T_PRED>
  static sp_version_relations_lattice_t
  create_sp_version_relations_lattice_at_pc(sp_version_relations_t const& spvr, map<graph_loc_id_t,expr_ref> const& locid2expr, sprel_map_pair_t const& sprel_map_pair, T_PC const& pp, graph_with_aliasing<T_PC,T_N,T_E,T_PRED> const& g)
  {
    return sp_version_relations_lattice_t(predicate_set_to_expr(sp_version_relations_to_preds(spvr, locid2expr), g.get_context()), construct_ml_to_is_absent_lb_ub_versions_at_pc(pp, sprel_map_pair, g));
  }

  memlabel_t tighten_mem_access_ml(memlabel_t const& in_ml, expr_ref const& addr, unsigned count) const;
  expr_ref determine_least_element(set<expr_ref> const& exprs) const;

  string to_string() const;
};

}
