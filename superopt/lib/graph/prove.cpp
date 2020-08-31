#include <fstream>

#include "expr/context_cache.h"
//#include "expr/expr_query_cache.h"
#include "expr/aliasing_constraints.h"
#include "expr/alignment_map.h"
#include "tfg/parse_input_eq_file.h"
#include "graph/query_decomposition.h"
#include "support/globals_cpp.h"
#include "support/c_utils.h"
#include "support/bv_const_expr.h"
#include "support/globals.h"
#include "expr/eval.h"
#include "eq/corr_graph.h"

namespace eqspace {


/*void
update_simplify_cache(context *ctx, expr_ref const &a, expr_ref const &b, unordered_set<predicate> const &lhs)
{
  if (is_expr_equal_syntactic(a, b)) {
    return;
  }
  autostop_timer func_timer(__func__);
  expr_ref from, to;
  if (ctx->expr_is_less_than(a, b)) {
    from = b;
    to = a;
  } else {
    from = a;
    to = b;
  }
  if (ctx->m_cache->m_simplify_using_proof_queries.count(to->get_id())) {
    autostop_timer func_timer(string(__func__) + "_identify_simpler_to");
    for (auto &pr : ctx->m_cache->m_simplify_using_proof_queries.at(to->get_id()).second) {
      unordered_set<predicate> const &set = pr.second;
      if (predicate_set_contains(lhs, set)) {
        to = pr.first;
      }
    }
  }

  {
    autostop_timer func_timer(string(__func__) + "_replace_all_froms_with_to");
    for (auto sm : ctx->m_cache->m_simplify_using_proof_queries) {
      for (auto &pr : sm.second.second) {
        unordered_set<predicate> const &set = pr.second;
        if (   pr.first == from
            && predicate_set_contains(lhs, set)) {
          pr.first = to;
        }
      }
    }
  }

  if (!ctx->m_cache->m_simplify_using_proof_queries.count(from->get_id())) {
    ctx->m_cache->m_simplify_using_proof_queries[from->get_id()] = make_pair(from, list<pair<expr_ref, unordered_set<predicate>>>());
  }
  ctx->m_cache->m_simplify_using_proof_queries.at(from->get_id()).second.push_back(make_pair(to, lhs));
}

expr_ref
get_simplified_expr_using_simplify_cache(context *ctx, expr_ref a, unordered_set<predicate> const &lhs)
{
  expr_ref ret = a;
  if (ctx->m_cache->m_simplify_using_proof_queries.count(a->get_id())) {
    for (auto &pr : ctx->m_cache->m_simplify_using_proof_queries.at(a->get_id()).second) {
      //set<pair<expr_ref, pair<expr_ref, expr_ref>>> const &set = pr.second;
      unordered_set<predicate> const &set = pr.second;
      if (   predicate_set_contains(lhs, set)
          && ctx->expr_is_less_than(pr.first, ret)) {
        ret = pr.first;
      }
    }
  }
  return ret;
}*/

//void
//predicate::simplify_using_lhs_set_and_precond(unordered_set<predicate> const &lhs_set, precond_t const &precond, sprel_map_pair_t const &sprel_map_pair, tfg_suffixpath_t const &src_suffixpath, pred_avail_exprs_t const &src_pred_avail_exprs)
//{
//  std::function<expr_ref (const string &, expr_ref const &)> f = [&lhs_set, &precond, &sprel_map_pair, &src_suffixpath, &src_pred_avail_exprs](string const &l, expr_ref const &e)
//  {
//    return expr_do_simplify_using_lhs_set_and_precond(e, true, lhs_set, precond, sprel_map_pair, src_suffixpath, src_pred_avail_exprs);
//  };
//  predicate new_pred = this->visit_exprs(f);
//  precond_t cur_precond = new_pred.get_precond();
//  if (precond.precond_implies(cur_precond)) {
//    //new_pred.set_edge_guard(edge_guard_t());
//    //new_pred.set_precond(precond_t());
//    new_pred.clear_precond();
//  }
//  *this = new_pred;
//}


/*map<expr_id_t, pair<expr_ref, expr_ref>>
lhs_set_get_submap(unordered_set<predicate> const &lhs_set, precond_t const &precond, context *ctx)
{
  consts_struct_t const &cs = ctx->get_consts_struct();
  map<expr_id_t, pair<expr_ref, expr_ref>> ret;
  for (const auto &t : lhs_set) {
    if (!precond.precond_implies(t.get_precond())) {
      continue;
    }
    expr_ref lhs = t.get_lhs_expr();
    expr_ref rhs = t.get_rhs_expr();
    if (lhs->is_var() && rhs->is_var()) {
      if (expr_is_consts_struct_constant(lhs, cs)) {
        ret[rhs->get_id()] = make_pair(rhs, lhs);
      } else if (expr_is_consts_struct_constant(rhs, cs)) {
        ret[lhs->get_id()] = make_pair(lhs, rhs);
      } else {
        expr_ref min = (lhs < rhs) ? lhs : rhs;
        expr_ref max = (lhs < rhs) ? rhs : lhs;
        ret[min->get_id()] = make_pair(min, max);
      }
    } else if (lhs->is_var()) {
      ret[lhs->get_id()] = make_pair(lhs, rhs);
    } else if (rhs->is_var()) {
      ret[rhs->get_id()] = make_pair(rhs, lhs);
    }
  }
  return ret;
}*/

//predicate_eval_on_counter_example assigns random values to unassigned vars
}
