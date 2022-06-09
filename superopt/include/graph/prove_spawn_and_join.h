#pragma once

#include "support/spawn_and_join.h"

#include "graph/prove.h"
#include "graph/prove_qd.h"

namespace eqspace {

template<typename T_PRECOND_TFG>
proof_result_t
prove_spawn_and_join(context *ctx, list<guarded_pred_t> const &lhs, precond_t const &precond, graph_edge_composition_ref<pc,tfg_edge> const& eg, sprel_map_pair_t const &sprel_map_pair, tfg_suffixpath_t const &src_suffixpath, avail_exprs_t const &src_avail_exprs, graph_memlabel_map_t const &memlabel_map, expr_ref const &src, expr_ref const &dst, query_comment const &comment/*, bool timeout_res*/, map<expr_id_t, pair<expr_ref, expr_ref>> const& concrete_address_submap, relevant_memlabels_t const& relevant_memlabels, aliasing_constraints_t const& alias_cons, T_PRECOND_TFG const &src_tfg/*, list<counter_example_t> &counter_examples*/)
{
  autostop_timer func_timer(__func__);
  ctx->increment_num_proof_queries();

  consts_struct_t const &cs = ctx->get_consts_struct();
  graph_symbol_map_t const &symbol_map = src_tfg.get_symbol_map();
  graph_locals_map_t const &locals_map = src_tfg.get_locals_map();
  //relevant_memlabels_t const& relevant_memlabels = src_tfg.graph_get_relevant_memlabels();

  expr_ref src_simplified, dst_simplified;

  //lhs_set1 = predicate::determinized_pred_set(lhs);
  list<shared_ptr<predicate const>> preds = get_uapprox_predicate_list_from_guarded_preds_and_graph_ec(lhs, eg);
  src_simplified = expr_simplify::expr_do_simplify_using_lhs_set_and_precond(src, true, preds, precond, sprel_map_pair, symbol_map, locals_map);
  dst_simplified = expr_simplify::expr_do_simplify_using_lhs_set_and_precond(dst, true, preds, precond, sprel_map_pair, symbol_map, locals_map);

  //CAUTION: any changes in this simplification/substitution also need to be repeated at the time of precond_expr computation in is_expr_equal_using_lhs_set_and_precond_helper

  DYN_DEBUG3(prove_debug, cout << __func__ << " " << __LINE__ << ":  before is_expr_equal_syntactic check, src_simplified: " << expr_string(src_simplified) << '\n');
  DYN_DEBUG3(prove_debug, cout << __func__ << " " << __LINE__ << ":  before is_expr_equal_syntactic check, dst_simplified: " << expr_string(dst_simplified) << '\n');

  if (expr_simplify::is_expr_equal_syntactic(src_simplified, dst_simplified)) {
    ctx->increment_num_proof_queries_answered_by_syntactic_check();
    DYN_DEBUG2(prove_debug, cout << __func__ << " " << __LINE__ << ": returning true because is_expr_equal_syntactic(src, dst)\n");
    return proof_result_t::proof_result_create(proof_status_proven, list<counter_example_t>(), true);
  }

  /*
  // substitute using rodata

  rodata_map_t rodata_map_pruned = rodata_map;
  map<expr_id_t, pair<expr_ref, expr_ref>> rodata_submap;
  if (rodata_map_pruned.prune_using_expr_pair(src_simplified, dst_simplified)) {
    rodata_submap = rodata_map_pruned.rodata_map_get_submap(ctx, src_tfg);
    lhs_set1 = lhs_set_substitute_using_submap(lhs_set1, rodata_submap, ctx);
    src_simplified = ctx->expr_substitute(src_simplified, rodata_submap);
    dst_simplified = ctx->expr_substitute(dst_simplified, rodata_submap);
  }

  // rodata substitution needed for precond_expr only now
  */

  //auto lhs_set1 = lhs;
  list<guarded_pred_t> sprel_preds = sprel_map_pair_get_predicates_for_non_var_locs(sprel_map_pair, ctx);

  list<guarded_pred_t> lhs_set1;
  lhs_set1 = predicate::lhs_set_sort(lhs, src_simplified, dst_simplified);
  list_append(lhs_set1, sprel_preds); // Later, in is_expr_equal_syntactic_using_lhs_set_and_precond, we add the var locs as well

  DYN_DEBUG4(prove_debug, cout << __func__ << " " << __LINE__ << ": before expr_eliminate_constructs_that_the_solver_cannot_handle, src_simplified: " << expr_string(src_simplified) << endl);
  DYN_DEBUG4(prove_debug, cout << __func__ << " " << __LINE__ << ": before expr_eliminate_constructs_that_the_solver_cannot_handle, dst_simplified: " << expr_string(dst_simplified) << endl);

  // we somehow need to call eliminate_constructs_that_solver_cannot_handle twice; needs investigation
  stats::get().get_timer(string(__func__) + ".eliminate_constructs_that_the_solver_cannot_handle1")->start();
  lhs_set1 = lhs_set_eliminate_constructs_that_the_solver_cannot_handle(lhs_set1, ctx);
  src_simplified = ctx->expr_eliminate_constructs_that_the_solver_cannot_handle(src_simplified);
  dst_simplified = ctx->expr_eliminate_constructs_that_the_solver_cannot_handle(dst_simplified);
  stats::get().get_timer(string(__func__) + ".eliminate_constructs_that_the_solver_cannot_handle1")->stop();

  DYN_DEBUG4(prove_debug, cout << __func__ << " " << __LINE__ << ":  after simplification, src_simplified: " << expr_string(src_simplified) << '\n');
  DYN_DEBUG4(prove_debug, cout << __func__ << " " << __LINE__ << ":  after simplification, dst_simplified: " << expr_string(dst_simplified) << '\n');

  if (expr_simplify::is_expr_equal_syntactic(src_simplified, dst_simplified)) {
    ctx->increment_num_proof_queries_answered_by_syntactic_check();
    DYN_DEBUG4(prove_debug, cout << __func__ << " " << __LINE__ << ": is_expr_equal_syntactic() returned true\n");
    return proof_result_t::proof_result_create(proof_status_proven, list<counter_example_t>(), true);
  }

  DYN_DEBUG4(prove_debug, cout << __func__ << " " << __LINE__ << ": lhs_set1 =\n"; for (auto p : lhs_set1) cout << __func__ << " " << __LINE__ << ": p = " << p.second->to_string(true) << endl);
  return prove_spawn_and_join_helper(ctx, lhs_set1, precond, eg, sprel_map_pair, src_suffixpath, src_avail_exprs, memlabel_map, src_simplified, dst_simplified, comment/*, timeout_res*/, concrete_address_submap, relevant_memlabels, alias_cons, src_tfg/*, counter_examples*/);
}

template<typename T_PRECOND_TFG>
proof_result_t
prove_spawn_and_join_helper(context *ctx, list<guarded_pred_t> const &lhs, precond_t const &precond, graph_edge_composition_ref<pc,tfg_edge> const& eg, sprel_map_pair_t const &sprel_map_pair, tfg_suffixpath_t const &src_suffixpath, avail_exprs_t const &src_avail_exprs, graph_memlabel_map_t const &memlabel_map, expr_ref const &src, expr_ref const &dst, query_comment const &comment/*, bool timeout_res*/, map<expr_id_t, pair<expr_ref, expr_ref>> const& concrete_address_submap, relevant_memlabels_t const& relevant_memlabels, aliasing_constraints_t const& alias_cons, T_PRECOND_TFG const &src_tfg/*, list<counter_example_t> &counter_examples*/)
{
  std::function prove_simple_func = [&](std::atomic<bool> &should_return)
  {
    auto ret = prove(ctx, lhs, precond, eg, sprel_map_pair, src_suffixpath, src_avail_exprs, memlabel_map, src, dst, comment/*, timeout_res*/, concrete_address_submap, relevant_memlabels, alias_cons, src_tfg, should_return);
    return ret;
  };
  std::function prove_qd_func = [&](std::atomic<bool> &should_return)
  {
    g_expect_no_deterministic_malloc_calls = true;
    auto ret = prove_qd(ctx, lhs, precond, eg, sprel_map_pair, src_suffixpath, src_avail_exprs, memlabel_map, src, dst, comment/*, timeout_res*/, concrete_address_submap, relevant_memlabels, alias_cons, src_tfg, should_return);
    g_expect_no_deterministic_malloc_calls = false;
    return ret;
  };

  //return spawn_and_join_first_and_cancel_others(prove_simple_func, prove_qd_func);
  std::atomic<bool> dummy = false;
  return prove_simple_func(dummy);
}

}
