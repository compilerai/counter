#pragma once

#include <list>
#include <fstream>

#include "support/globals_cpp.h"
#include "support/globals.h"
#include "support/utils.h"
#include "support/query_comment.h"

#include "expr/expr.h"
#include "expr/context.h"
#include "expr/aliasing_constraints.h"
#include "expr/sprel_map_pair.h"
#include "expr/proof_result.h"
#include "expr/solver_interface.h"

#include "graph/avail_exprs.h"
#include "graph/query_decomposition.h"
#include "graph/commonMEM_opt_res.h"

namespace eqspace {


//expr_ref sprel_map_pair_and_var_locs(sprel_map_pair_t const& sprel_map_pair, expr_ref const &in);
//bool lhs_set_includes_nonstack_mem_equality(list<predicate_ref> const &lhs_set, memlabel_assertions_t const& mlasserts, map<expr_id_t, expr_ref> &input_mem_exprs);


//expr_ref
//sprel_map_pair_and_var_locs(sprel_map_pair_t const& sprel_map_pair, expr_ref const &in)
//{
//  expr_ref cur = in;
//  context* ctx = in->get_context();
//  for (auto const& s : sprel_map_pair.sprel_map_pair_get_submap()) {
//    if (s.second.first->is_var()) {
//      cur = expr_and(cur, ctx->mk_eq(s.second.first, s.second.second));
//    }
//  }
//  return cur;
//}
//expr_ref avail_exprs_and(map<graph_loc_id_t, expr_ref> const &avail_exprs, expr_ref const &in, tfg const &t);
//bool expr_set_lhs_prove_src_dst(context *ctx, unordered_set<predicate_ref>const &lhs, edge_guard_t const &guard, expr_ref src, expr_ref dst, query_comment const &qc, bool timeout_res, counter_example_t &counter_example, consts_struct_t const &cs);

//void update_simplify_cache(context *ctx, expr_ref const &a, expr_ref const &b, list<predicate_ref> const &lhs);
//expr_ref get_simplified_expr_using_simplify_cache(context *ctx, expr_ref const &a, list<predicate_ref> const &lhs);
//bool evaluate_counter_example_on_prove_query(context *ctx, list<predicate_ref> const &lhs, expr_ref const &src, expr_ref const &dst, counter_example_t &counter_example);
//bool expr_triple_set_contains(list<predicate_ref> const &haystack, list<predicate_ref> const &needle);

//unordered_set<predicate_ref>local_sprel_expr_guesses_get_pred_set(local_sprel_expr_guesses_t const &g, context* ctx);

//map<expr_id_t, pair<expr_ref, expr_ref>> lhs_set_get_submap(unordered_set<predicate_ref>const &lhs_set, precond_t const &precond, context *ctx);

//template<typename T_PRECOND_TFG>
//void output_lhs_set_guard_etc_and_src_dst_to_file(ostream &fo, predicate_set_t const &lhs_set, precond_t const& precond, sprel_map_pair_t const &sprel_map_pair, tfg_suffixpath_t const &src_suffixpath, avail_exprs_t const &src_avail_exprs, aliasing_constraints_t const& alias_cons, graph_memlabel_map_t const &memlabel_map, expr_ref src, expr_ref dst, map<expr_id_t, pair<expr_ref, expr_ref>> const& concrete_address_submap/*, memlabel_assertions_t const& mlasserts*/, T_PRECOND_TFG const &src_tfg);

commonMEM_opt_res_t
perform_commonMEM_optimization(context* ctx, expr_ref& lhs_expr, expr_ref& simplified_rhs, relevant_memlabels_t const& relevant_memlabels, list<guarded_pred_t> const &lhs_set);

//proof_status_t solver_res_to_proof_status(solver_res sr);


template<typename T_PRECOND_TFG>
//solver::solver_res
proof_result_t
is_expr_equal_using_lhs_set_and_precond_helper(context *ctx, list<guarded_pred_t> const &lhs_set, precond_t const &precond, graph_edge_composition_ref<pc, tfg_edge> const& eg, sprel_map_pair_t const &sprel_map_pair, tfg_suffixpath_t const &src_suffixpath, avail_exprs_t const &src_avail_exprs, graph_memlabel_map_t const &memlabel_map, expr_ref const &src, expr_ref const &dst, const query_comment& qc/*, list<counter_example_t> &counter_examples*/, map<expr_id_t, pair<expr_ref, expr_ref>> const& concrete_address_submap/*, dshared_ptr<memlabel_assertions_t> const& mlasserts*/, relevant_memlabels_t const& relevant_memlabels, aliasing_constraints_t const& alias_cons, T_PRECOND_TFG const &src_tfg)
{
  autostop_timer ft(__func__);

  DYN_DEBUG3(houdini, cout << __func__ << " " << __LINE__ << ": src = " << expr_string(src) << endl);
  DYN_DEBUG3(houdini, cout << __func__ << " " << __LINE__ << ": dst = " << expr_string(dst) << endl);

  list<counter_example_t> counter_examples;

  if (expr_simplify::is_expr_equal_syntactic(src, dst)) {
    ctx->increment_num_proof_queries_answered_by_syntactic_check();
    return proof_result_t::proof_result_create(proof_status_t::proof_status_proven, {}, true);
    //return solver::solver_res_true;
  }

  DYN_DEBUG(smt_query_dump,
    stringstream ss;

    ss << g_query_dir << "/" << IS_EXPR_EQUAL_USING_LHS_SET_FILENAME_PREFIX << "." << qc.to_string();
    string filename = ss.str();
    std::ofstream fo;
    fo.open(filename.data());
    ASSERT(fo);
    output_lhs_set_guard_etc_and_src_dst_to_file(fo, lhs_set, precond, eg, sprel_map_pair, src_suffixpath, src_avail_exprs, memlabel_map, src, dst, concrete_address_submap/*mlasserts*/, relevant_memlabels, alias_cons, src_tfg);
    fo.close();
    //g_query_files.insert(filename);
    //expr_num = (expr_num + 1) % 2; //don't allow more than 500 files per process
    cout << "is_expr_equal_using_lhs_set filename " << filename << endl;
  );

  expr_ref rhs = ctx->mk_eq(src, dst);

  autostop_timer func_timer(string(__func__) + ".syntactic_check_failed");
  CPP_DBG_EXEC(COUNTERS_ENABLE, stats::get().inc_counter(string(__func__) + ".queries.total"));

  //expr_query_cache_t<pair<bool, counter_example_t>> &cache = ctx->m_cache->m_is_expr_equal_using_lhs_set;
  //if (!ctx->disable_caches()) {
  //  bool lb_found, ub_found;
  //  pair<bool, counter_example_t> lb = make_pair(false, counter_example_t(ctx));
  //  pair<bool, counter_example_t> ub = make_pair(false, counter_example_t(ctx));
  //  cache.find_bounds(lhs_set, precond, sprel_map_pair, src_suffixpath, src_avail_exprs, rhs, lb_found, lb, ub_found, ub);
  //  if (ub_found && ub.first) {
  //    CPP_DBG_EXEC2(HOUDINI, cout << __func__ << " " << __LINE__ << ": returning true from cache\n");
  //    return solver::solver_res_true;
  //  }
  //  if (lb_found && !lb.first) {
  //    counter_example = lb.second;
  //    CPP_DBG_EXEC2(HOUDINI, cout << __func__ << " " << __LINE__ << ": returning false from cache\n");
  //    return solver::solver_res_false;
  //  }

  //  ASSERT(!lb_found || !ub_found || lb.first || !ub.first);
  //}
  //stats::get().inc_counter(string(__func__) + ".cache.misses");

  //autostop_timer func_timer2(string(__func__) + "_miss");

  DYN_DEBUG2(prove_debug, cout << __func__ << " " << __LINE__ << ": rhs = " << expr_string(rhs) << endl);
  expr_ref simplified_rhs = rhs; //expr_do_simplify_using_lhs_set_and_precond(rhs, true, lhs_set, precond, sprel_map_pair, src_suffixpath, src_avail_exprs);
  //DYN_DEBUG2(prove_debug, cout << __func__ << " " << __LINE__ << ": after simplify, rhs = " << expr_string(simplified_rhs) << endl);

  //autostop_timer func_timer3(string(__func__) + "_miss_after_simplify");
  expr_ref precond_expr = precond.precond_get_expr(ctx/*, true*/);
  expr_ref guard_expr = src_tfg.graph_edge_composition_get_simplified_edge_cond(eg);
  precond_expr = expr_and(precond_expr, guard_expr);
  //precond_expr = ctx->expr_substitute_using_sprel_map_pair(precond_expr, sprel_map_pair);
  //precond_expr = ctx->expr_simplify_using_sprel_pair_and_memlabel_maps(precond_expr, sprel_map_pair, memlabel_map/*, relevant_addr_refs, symbol_map, locals_map, src_tfg.get_argument_regs()*/);
  //map<expr_id_t, pair<expr_ref, expr_ref>> const &rodata_submap = rodata_map.rodata_map_get_submap(ctx, src_tfg);
  //precond_expr = ctx->expr_substitute(precond_expr, rodata_submap);
  precond_expr = ctx->expr_eliminate_constructs_that_the_solver_cannot_handle(precond_expr);
  DYN_DEBUG2(prove_debug, cout << __func__ << " " << __LINE__ << ": precond = " << precond.precond_to_string() << endl);
  DYN_DEBUG2(prove_debug, cout << __func__ << " " << __LINE__ << ": before simplify: precond_expr = " << expr_string(precond_expr) << endl);
  //precond_expr = expr_do_simplify_using_lhs_set_and_precond(precond_expr, true, lhs_set, precond, sprel_map_pair, src_suffixpath, src_avail_exprs);
  DYN_DEBUG2(prove_debug, cout << __func__ << " " << __LINE__ << ": after simplify using lhs set and precond: precond_expr = " << expr_string(precond_expr) << endl);
  if (expr_simplify::is_expr_equal_syntactic(precond_expr, expr_false(ctx))) {
    ctx->increment_num_proof_queries_answered_by_syntactic_check();
    return proof_result_t::proof_result_create(proof_status_t::proof_status_proven, {}, true);
    //return solver::solver_res_true;
  }

  autostop_timer func_timer10(string(__func__) + "_miss_after_precond_is_false");
  expr_ref pred_set_expr = guarded_predicate_set_and(lhs_set, expr_true(ctx), precond, /*rodata_submap,*/ src_tfg);
  DYN_DEBUG4(prove_debug, cout << __func__ << " " << __LINE__ << ": lhs_expr:\n " << expr_string(pred_set_expr) << endl);

  //unsubstituted edgecond works for SSA representation (which we assume for src_tfg).
  //substituted edgecond would create long expressions; unsubstituted edge conds are always correct in SSA but may not be useful. Sometimes they are useful, and so let's use them in our lhs set
  expr_ref suffixpath_expr = src_tfg.tfg_suffixpath_get_expr(src_suffixpath);
  suffixpath_expr = ctx->expr_eliminate_constructs_that_the_solver_cannot_handle(suffixpath_expr);

  expr_ref avail_expr = src_avail_exprs.avail_exprs_and(expr_true(ctx), src_tfg.get_locid2expr_map()/*, src_suffixpath*/);
  avail_expr = ctx->expr_eliminate_constructs_that_the_solver_cannot_handle(avail_expr);

  expr_ref sprel_map_pair_expr = sprel_map_pair_and_var_locs(sprel_map_pair, expr_true(ctx)); // in prove, we add the non-var locs

  // -- all simplifications should have been performed before this --

  expr_ref lhs_expr = pred_set_expr;
  lhs_expr = expr_and(precond_expr, lhs_expr);
  lhs_expr = expr_and(suffixpath_expr, lhs_expr);
  lhs_expr = expr_and(avail_expr, lhs_expr);
  lhs_expr = expr_and(sprel_map_pair_expr, lhs_expr);

  autostop_timer func_timer4(string(__func__) + "_miss_after_simplify_and_add_auxiliary_structures");
  DYN_DEBUG4(prove_debug, cout << __func__ << " " << __LINE__ << ": lhs_expr:\n" << expr_string(lhs_expr) << endl);

  DYN_DEBUG4(prove_debug, cout << __func__ << " " << __LINE__ << ": aliasing constraints =\n" << alias_cons.to_string_for_eq() << endl);
  expr_ref alias_cons_expr = alias_cons.convert_to_expr(ctx);
  DYN_DEBUG4(prove_debug, cout << __func__ << " " << __LINE__ << ": aliasing constraints expr = " << expr_string(alias_cons_expr) << endl);

  lhs_expr = expr_and(alias_cons_expr, lhs_expr);

  map<expr_id_t,pair<expr_ref,expr_ref>> const& memlabel_range_submap = concrete_address_submap;
  lhs_expr = ctx->expr_substitute(lhs_expr, memlabel_range_submap);
  lhs_expr = expr_and(ctx->submap_get_expr(memlabel_range_submap), lhs_expr);
  simplified_rhs = ctx->expr_substitute(simplified_rhs, memlabel_range_submap);
  //{//assertcheck
  //  expr_ref ml_assertions_substituted = ctx->expr_substitute(memlabel_assertions, memlabel_range_submap);
  //  ml_assertions_substituted = expr_evaluates_to_constant(ml_assertions_substituted);
  //  ASSERT(ml_assertions_substituted->is_const());
  //  if (!ml_assertions_substituted->get_bool_value()) {
  //    cout << __func__ << " " << __LINE__ << ": printing memlabel range submap\n";
  //    for (auto const &s : memlabel_range_submap) {
  //      cout << expr_string(s.second.first) << " --> " << expr_string(s.second.second) << endl;
  //    }
  //  }
  //  ASSERT(ml_assertions_substituted->get_bool_value());
  //}

// part of generated eqclass inductive preds 
  //expr_ref memlabel_rangeval_assertions = mlasserts->get_assertion();
  //if (memlabel_range_submap.empty()) {
  //  memlabel_rangeval_assertions = generate_range_assertions_for_memlabels(ctx, relevant_memlabels, rodata_map);
  //  CPP_DBG_EXEC3(HOUDINI, cout << __func__ << " " << __LINE__ << ": memlabel_rangeval_assertions = " << ctx->expr_to_string_table(memlabel_rangeval_assertions) << endl);
  //} else {
  //  memlabel_rangeval_assertions = ctx->submap_generate_assertions(memlabel_range_submap);
  //}
  //ASSERT(memlabel_rangeval_assertions);

  //lhs_expr = expr_and(memlabel_assertions, lhs_expr);
  //lhs_expr = expr_and(memlabel_rangeval_assertions, lhs_expr);

  map<expr_ref, set<expr_ref>> nonstk_memvar_to_orig_mem_map;
  map<expr_ref, expr_ref> stk_memvar_to_orig_dst_memvar_map;
  commonMEM_opt_res_t commonMEM_opt_res = perform_commonMEM_optimization(ctx, lhs_expr, simplified_rhs, relevant_memlabels, lhs_set);

  nonstk_memvar_to_orig_mem_map = commonMEM_opt_res.get_nonstck_memvar_to_orig_mem_map();
  stk_memvar_to_orig_dst_memvar_map = commonMEM_opt_res.get_stck_memvar_to_orig_dst_memvar_map();

  if (ctx->get_config().enable_semantic_simplification_in_prove_path) {
    autostop_timer semantic_simplify_timer(string(__func__) + ".expr_semantically_simplify_using_lhs_expr");
    simplified_rhs = expr_simplify::expr_semantically_simplify_using_lhs_expr(lhs_expr, simplified_rhs, relevant_memlabels);
  }

  DYN_DEBUG2(prove_debug, cout << __func__ << " " << __LINE__ << ": lhs_expr =\n" << ctx->expr_to_string_table(lhs_expr) << endl);
  DYN_DEBUG2(prove_debug, cout << __func__ << " " << __LINE__ << ": simplified_rhs =\n" << ctx->expr_to_string_table(simplified_rhs) << endl);
  expr_ref e = expr_implies(lhs_expr, simplified_rhs);
  DYN_DEBUG4(prove_debug, cout << __func__ << " " << __LINE__ << ": e = " << expr_string(e) << endl);

  autostop_timer func_timer9(string(__func__) + "_miss_after_aliasing_constraints_generation");

  //autostop_timer func_timer5(string(__func__) + "_miss_after_writing_out_is_expr_equal_using_lhs_set_filename");
  /*if (ctx->context_counter_example_exists(e, counter_example, cs)) {
    autostop_timer func_timer8(string(__func__) + "_miss_counter_example_exists");
    //m_num_proof_queries_answered_by_counter_examples++;
    ctx->increment_num_proof_queries_answered_by_counter_examples();
    //ASSERT2(!e->is_provable(qc, timeout_res, counter_example));
    CPP_DBG_EXEC2(HOUDINI, cout << __func__ << " " << __LINE__ << ": returning false because counter_example exists:\n");
    CPP_DBG_EXEC3(HOUDINI,
        cout << __func__ << ' ' << __LINE__ << ": evaluation: :\n";
        for (auto const& ev : evaluate_counter_example_on_expr_map(ctx->expr_to_expr_map(e), counter_example))
          cout << ev.first << ": " << expr_string(ev.second) << endl
    );
    if (!ctx->disable_caches()) {
      cache.add(lhs_set, precond, sprel_map_pair, src_suffixpath, src_avail_exprs, rhs, make_pair(false, counter_example));
    }
    return solver::solver_res_false;
  }*/

  //autostop_timer func_timer6(string(__func__) + "_miss_after_checking_counter_example_exists");
  //set<memlabel_ref> const& relevant_memlabels_set = relevant_memlabels.get_relevant_memlabels();

  proof_result_t ret = ctx->get_solver()->expr_is_provable(e, qc, relevant_memlabels/*, counter_examples*/);
  counter_examples = ret.get_counterexamples();

  autostop_timer func_timer7(string(__func__) + "_miss_after_expr_is_provable");
  if (ret.get_proof_status() == proof_status_t::proof_status_disproven) {
    for (auto& ce : counter_examples) {
      DYN_DEBUG(mem_fuzzing, cout << __func__ << " " << __LINE__ <<" CE before mem fuzzing" << endl;
          cout << ce.counter_example_to_string() << endl;
      );
      ce.ce_perform_memory_fuzzing(e, ctx, relevant_memlabels);
      DYN_DEBUG(mem_fuzzing, cout << __func__ << " " << __LINE__ <<" CE after mem fuzzing" << endl;
          cout << ce.counter_example_to_string() << endl;
      );
      if (!ce.is_empty()) {
        DYN_DEBUG(prove_debug, cout << __func__ << " " << __LINE__ <<" CE before nonstack reconcilate" << endl;
            cout << ce.counter_example_to_string() << endl;
        );
        for(auto const& [nonstack_memvar, input_mem_exprs] : nonstk_memvar_to_orig_mem_map) {
          ASSERT(nonstack_memvar);
          ce.add_input_memvars_using_nonstack_memvar(input_mem_exprs, nonstack_memvar);
          DYN_DEBUG(prove_debug, cout << __func__ << " " << __LINE__ <<" CE after nonstack reconcilate for nonstack_memvar: " << expr_string(nonstack_memvar) << endl;
              cout << ce.counter_example_to_string() << endl;
          );
        } 
        for(auto const& [stk_mem, orig_dst_memvar] : stk_memvar_to_orig_dst_memvar_map) {
          DYN_DEBUG(prove_debug, cout << __func__ << " " << __LINE__ <<" CE before stack reconcilate for stack_memvar: " << expr_string(stk_mem) << ", orig_dst_mem:" << expr_string(orig_dst_memvar) << endl;
              cout << ce.counter_example_to_string() << endl;
          );
          ce.reconciliate_stack_mem_in_counter_example(stk_mem, orig_dst_memvar);
          DYN_DEBUG(prove_debug, cout << __func__ << " " << __LINE__ <<" CE after stack reconcilate for stack_memvar: " << expr_string(stk_mem) << endl;
              cout << ce.counter_example_to_string() << endl;
          );
        }
      }
    }
  }

  return proof_result_t::proof_result_create(ret.get_proof_status(), counter_examples, ret.was_trivially_resolved(), ret.get_num_quantifiers(), ret.involved_fp_to_ieee_bv(), commonMEM_opt_res.stack_modeled_as_separate_mem(), commonMEM_opt_res.nonstack_modeled_as_common_mem());
  //return ret;
}

template<typename T_PRECOND_TFG>
proof_result_t
is_expr_equal_using_lhs_set_and_precond_with_query_decomposition(context *ctx, list<guarded_pred_t> const &lhs_set, precond_t const &precond, graph_edge_composition_ref<pc, tfg_edge> const& eg, sprel_map_pair_t const &sprel_map_pair, tfg_suffixpath_t const &src_suffixpath, avail_exprs_t const &src_avail_exprs, graph_memlabel_map_t const &memlabel_map, /*rodata_map_t const &rodata_map,*/ expr_ref const &src, expr_ref const &dst, const query_comment& qc/*, list<counter_example_t> &counter_examples*/, map<expr_id_t, pair<expr_ref, expr_ref>> const& concrete_address_submap, relevant_memlabels_t const& relevant_memlabels, aliasing_constraints_t const& rhs_alias_cons, T_PRECOND_TFG const &src_tfg)
{

  query_decomposer obj(ctx, src, dst);
  return obj.find_mappings_and_perform_query(lhs_set, precond, eg, sprel_map_pair, src_suffixpath, src_avail_exprs, memlabel_map, qc/*, counter_examples*/, concrete_address_submap, relevant_memlabels, rhs_alias_cons, src_tfg);
}

template<typename T_PRECOND_TFG>
proof_result_t
is_expr_equal_using_lhs_set_and_precond(context *ctx, list<guarded_pred_t> lhs_set, precond_t const &precond, graph_edge_composition_ref<pc,tfg_edge> const& eg, sprel_map_pair_t const &sprel_map_pair, tfg_suffixpath_t const &src_suffixpath, avail_exprs_t const &src_avail_exprs, graph_memlabel_map_t const &memlabel_map, expr_ref src, expr_ref dst, const query_comment& qc/*, bool timeout_res*//*, list<counter_example_t> &counter_examples*/, map<expr_id_t, pair<expr_ref, expr_ref>> const& concrete_address_submap, relevant_memlabels_t const& relevant_memlabels, aliasing_constraints_t const& rhs_alias_cons, T_PRECOND_TFG const &src_tfg, std::atomic<bool>& should_return_sharedvar)
{
  bool is_axpred_query = ctx->src_dst_represent_axpred_query(src, dst);

  if (!is_axpred_query) {
    lhs_set = guarded_predicate_set_eliminate_axpreds(lhs_set);
  }

  proof_result_t ret = is_expr_equal_using_lhs_set_and_precond_helper(ctx, lhs_set, precond, eg, sprel_map_pair, src_suffixpath, src_avail_exprs, memlabel_map,
                                                                          src, dst, qc/*, counter_examples*/, concrete_address_submap, relevant_memlabels, rhs_alias_cons, src_tfg);
  //DYN_DEBUG(prove_debug, cout << __func__ << " " << __LINE__ << " Result of is_expr_equal_using_lhs_set_and_precond_helper is: " << solver::solver_res_to_string(ret) << endl);

  if (ret.get_proof_status() == proof_status_t::proof_status_timeout && ctx->get_config().enable_query_decomposition) {
    stats::get().inc_counter("query-decomposition-invoked");
    ret = is_expr_equal_using_lhs_set_and_precond_with_query_decomposition(ctx, lhs_set, precond, eg, sprel_map_pair, src_suffixpath, src_avail_exprs, memlabel_map, 
                                                                           src, dst, qc/*, counter_examples*/, concrete_address_submap, relevant_memlabels, rhs_alias_cons, src_tfg);
    if (ret.get_proof_status() == proof_status_t::proof_status_timeout) {
      stats::get().inc_counter("query-decomposition-timed-out");
    }
    DYN_DEBUG(prove_debug, cout << __func__ << " " << __LINE__ << " Result of Query Decomposition is: timeout"/* << solver::solver_res_to_string(ret)*/ << endl);
  }

  if (is_axpred_query && (ret.get_proof_status() == proof_status_t::proof_status_timeout || ret.get_proof_status() == proof_status_t::proof_status_timeout)) {
    auto is_expr_equal_fn = [&](const expr_ref &src) {
      return is_expr_equal_using_lhs_set_and_precond_helper(ctx, lhs_set, precond, eg, sprel_map_pair, src_suffixpath, src_avail_exprs, memlabel_map,
                                                            src, dst, qc/*, counter_examples*/, concrete_address_submap, relevant_memlabels, rhs_alias_cons, src_tfg).get_proof_status() ==
             proof_status_t::proof_status_proven;
    };
    auto trace_fn = [&](const expr_ref &axpred, std::size_t cur_depth, std::size_t max_depth, bool is_succ) {
      ctx->get_axpred_context().debug() << "unif-trace-begin " << cur_depth << "/" << max_depth << "\n"
                                        << ctx->expr_to_string_table(axpred) << "\n"
                                        << "unif-trace-end" << std::endl;
      if (is_succ) {
        ctx->get_axpred_context().debug() << "succ" << std::endl;
      }
    };

    solver_res solver_ret = ctx->get_axpred_context().attemptUnifySolve(is_expr_equal_fn, trace_fn, src, 1);
    ret = proof_result_t::proof_result_create(proof_result_t::solver_res_to_proof_status(solver_ret), {});
  }

  return ret;
}

//static unordered_set<predicate>
//lhs_set_simplify_until_fixpoint(unordered_set<predicate> const &lhs_in, precond_t const &precond, sprel_map_pair_t const &sprel_map_pair, tfg_suffixpath_t const &src_suffixpath, avail_exprs_t const &src_avail_exprs)
//{
//  autostop_timer func_timer1(__func__);
//
//  if (lhs_in.empty())
//    return lhs_in;
//
//  context* ctx = lhs_in.begin()->get_lhs_expr()->get_context();
//  unordered_map<tuple<unordered_set<predicate>, precond_t, sprel_map_pair_t, tfg_suffixpath_t, avail_exprs_t>,unordered_set<predicate>> &cache = ctx->m_cache->m_lhs_simplify_until_fixpoint;
//  auto tup = make_tuple(lhs_in, precond, sprel_map_pair, src_suffixpath, src_avail_exprs);
//  if (!ctx->disable_caches()) {
//    auto fitr = cache.find(tup);
//    if (fitr != cache.end()) {
//      stats::get().inc_counter(string(__func__)+".cache.hits");
//      return fitr->second;
//    }
//  }
//
//  autostop_timer func_timer(string(__func__)+".miss");
//  unordered_set<predicate> ret = lhs_in;
//  unordered_set<predicate> new_ret;
//  bool something_changed;
//  do {
//    something_changed = false;
//    for (const auto &t : ret) {
//      unordered_set<predicate> const &lhs_without_t = ret;
//      //auto num_erased = lhs_without_t.erase(t); //uncommenting this makes this code really slow
//      //ASSERT(num_erased > 0);
//      predicate new_pred = t;
//      new_pred.simplify_using_lhs_set_and_precond(lhs_without_t, precond, sprel_map_pair, src_suffixpath, src_avail_exprs);
//      //new_ret.insert(make_pair(new_guard, make_pair(new_src, new_dst)));
//      new_ret.insert(new_pred);
//      if (!new_pred.equals(t)) {
//        something_changed = true;
//      }
//    }
//    ret.swap(new_ret);
//    new_ret.clear();
//  } while (something_changed);
//
//  if (!ctx->disable_caches()) {
//    cache.insert(make_pair(tup,ret));
//  }
//  return ret;
//}

/*static unordered_set<predicate>
lhs_set_mask_fcall_mem_argument_with_callee_readable_memlabel(unordered_set<predicate> const &lhs_in, context *ctx)
{
  unordered_set<predicate> ret;
  for (const auto &t : ret) {
    predicate new_pred = t;
    new_pred.pred_mask_fcall_mem_argument_with_callee_readable_memlabel();
    ret.insert(new_pred);
  }
  return ret;
}*/

/*static unordered_set<predicate>
lhs_set_fcall_orig_memvar_to_array_constant_and_memlabels_to_heap(unordered_set<predicate> const &lhs_set, context *ctx)
{
  unordered_set<predicate> ret;
  for (auto p : lhs_set) {
    predicate pnew = p.pred_set_fcall_orig_memvar_to_array_constant_and_memlabels_to_heap(ctx);
    ret.insert(pnew);
  }
  return ret;
}*/

//static unordered_set<predicate>
//lhs_set_simplify_using_sprel_pair_and_memlabel_map(unordered_set<predicate> const &lhs, sprel_map_pair_t const &sprel_map_pair, graph_memlabel_map_t const &memlabel_map/*, set<cs_addr_ref_id_t> const &relevant_addr_refs, graph_symbol_map_t const &symbol_map, graph_locals_map_t const &locals_map, map<string, expr_ref> const &argument_regs, context *ctx*/)
//{
//  autostop_timer func_timer(__func__);
//  unordered_set<predicate> ret;
//  for (auto p : lhs) {
//    predicate pnew = p.pred_simplify_using_sprel_pair_and_memlabel_map(/*ctx,*/sprel_map_pair, memlabel_map/*, relevant_addr_refs, symbol_map, locals_map, argument_regs*/);
//    //cout << __func__ << " " << __LINE__ << ": changing from " << p.to_string(true) << " to " << pnew.to_string(true) << endl;
//    ret.insert(pnew);
//  }
//  return ret;
//}

//list<predicate_ref>
//lhs_set_substitute_using_submap(list<predicate_ref> const &lhs, map<expr_id_t, pair<expr_ref, expr_ref>> const &submap, context *ctx)
//{
//  autostop_timer func_timer(__func__);
//  list<predicate_ref> ret;
//  for (auto const& p : lhs) {
//    predicate_ref pnew = p->pred_substitute_using_submap(ctx, submap);
//    pnew = pnew->simplify();
//    if (pnew->predicate_is_trivial()) {
//      continue;
//    }
//    //cout << __func__ << " " << __LINE__ << ": changing from " << p.to_string(true) << " to " << pnew.to_string(true) << endl;
//    ret.push_back(pnew);
//  }
//  return ret;
//}
//
//
//
//list<predicate_ref>
//lhs_set_eliminate_constructs_that_the_solver_cannot_handle(list<predicate_ref> const &lhs_set, context *ctx)
//{
//  autostop_timer func_timer(__func__);
//  list<predicate_ref> ret;
//  for (auto const& p : lhs_set) {
//    predicate_ref pnew = p->pred_eliminate_constructs_that_the_solver_cannot_handle(ctx);
//    //cout << __func__ << " " << __LINE__ << ": pnew = " << pnew.to_string(true) << endl;
//    ret.push_back(pnew);
//  }
//  return ret;
//}

template<typename T_PRECOND_TFG>
proof_result_t
prove(context *ctx, list<guarded_pred_t> const &lhs, precond_t const &precond, graph_edge_composition_ref<pc,tfg_edge> const& eg, sprel_map_pair_t const &sprel_map_pair, tfg_suffixpath_t const &src_suffixpath, avail_exprs_t const &src_avail_exprs, graph_memlabel_map_t const &memlabel_map, expr_ref const &src, expr_ref const &dst, query_comment const &comment/*, bool timeout_res*//*, graph_symbol_map_t const &symbol_map, graph_locals_map_t const &locals_map*/, map<expr_id_t, pair<expr_ref, expr_ref>> const& concrete_address_submap, relevant_memlabels_t const& relevant_memlabels, aliasing_constraints_t const& alias_cons, T_PRECOND_TFG const &src_tfg, std::atomic<bool>& should_return_sharedvar)
{
  //list<counter_example_t> counter_examples;
  //consts_struct_t const &cs = ctx->get_consts_struct();
  proof_result_t proof_result = is_expr_equal_using_lhs_set_and_precond(ctx, lhs/*_set1*/, precond, eg, sprel_map_pair, src_suffixpath, src_avail_exprs, memlabel_map, src, dst, comment/*, timeout_res*//*, counter_examples*/, concrete_address_submap, relevant_memlabels, alias_cons, src_tfg/*, cs*/, should_return_sharedvar);
  //auto ret = solver_res_to_proof_status(ret_solver_res);

  //for (auto& ce : counter_examples) {
  //  //ce.set_rodata_submap(rodata_submap);
  //  ce.counter_example_set_local_sprel_expr_assumes(precond.get_local_sprel_expr_assumes());
  //}

  //DYN_DEBUG3(prove_debug, cout << __func__ << " " << __LINE__ << ": returning " << proof_status_to_string(ret) << endl);
  //return proof_status_t(ret, counter_examples);
  return proof_result;
}

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
//predicate::simplify_using_lhs_set_and_precond(unordered_set<predicate> const &lhs_set, precond_t const &precond, sprel_map_pair_t const &sprel_map_pair, tfg_suffixpath_t const &src_suffixpath, avail_exprs_t const &src_avail_exprs)
//{
//  std::function<expr_ref (const string &, expr_ref const &)> f = [&lhs_set, &precond, &sprel_map_pair, &src_suffixpath, &src_avail_exprs](string const &l, expr_ref const &e)
//  {
//    return expr_do_simplify_using_lhs_set_and_precond(e, true, lhs_set, precond, sprel_map_pair, src_suffixpath, src_avail_exprs);
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

#if 0
template<typename T_PRECOND_TFG>
void
output_lhs_set_guard_etc_and_src_dst_to_file(ostream &fo, list<guarded_pred_t> const &lhs_set, precond_t const &precond, graph_edge_composition_ref<pc,tfg_edge> const& eg, sprel_map_pair_t const &sprel_map_pair, shared_ptr<tfg_edge_composition_t> const &src_suffixpath, avail_exprs_t const &src_avail_exprs, /*rodata_map_t const &rodata_map,*/ aliasing_constraints_t const& alias_cons, expr_ref const &src, expr_ref const &dst, T_PRECOND_TFG const &src_tfg)
{
  autostop_timer func_timer(__func__);
  fo << src_tfg.graph_to_string() << endl;
  size_t pred_num = 0;
  context *ctx = src->get_context();
  for (auto l : lhs_set) {
    stringstream ss;
    ss << "=lhs-guarded-pred" << pred_num;
    fo << ss.str() << endl;
    pred_num++;
    guarded_pred_to_stream(fo, l, ss.str());
    //fo << l->to_string_for_eq(true) << endl;
  }
  fo << "=edgeguard" << endl;
  //fo << precond.precond_get_guard().edge_guard_to_string_for_eq() << endl;
  if (eg) {
    eg->graph_edge_composition_to_stream(fo, "");
  } else {
    fo << "";
  }
  //fo << "=lsprels" << endl;
  //fo << precond.get_local_sprel_expr_assumes().to_string_for_eq(true) << endl;
  fo << "=sprel_map_pair" << endl;
  fo << sprel_map_pair.to_string_for_eq();
  fo << "=src_suffixpath" << endl;
  //fo << (src_suffixpath ? src_suffixpath->graph_edge_composition_to_string() : "(" EPSILON ")") << endl;
  src_suffixpath->graph_edge_composition_to_stream(fo, "=src_suffixpath");
  fo << "=src_avail_exprs" << endl;
  //fo << src_avail_exprs.avail_exprs_to_string();
  src_avail_exprs.avail_exprs_to_stream(fo, src_tfg.get_locid2expr_map());
  //fo << "=rodata_map" << endl;
  //fo << rodata_map.to_string_for_eq();
  fo << "=aliasing_constraints" << endl;
  fo << alias_cons.to_string_for_eq();
  fo << "=src" << endl;
  fo << ctx->expr_to_string_table(src, true) << endl;
  fo << "=dst" << endl;
  fo << ctx->expr_to_string_table(dst, true) << endl;
}
#endif

template<typename T_PRECOND_TFG>
void
output_lhs_set_guard_etc_and_src_dst_to_file(ostream &fo, list<guarded_pred_t> const &lhs_set, precond_t const& precond, graph_edge_composition_ref<pc,tfg_edge> const& eg, sprel_map_pair_t const &sprel_map_pair, shared_ptr<tfg_edge_composition_t> const &src_suffixpath, avail_exprs_t const &src_avail_exprs, graph_memlabel_map_t const &memlabel_map, expr_ref src, expr_ref dst, map<expr_id_t, pair<expr_ref, expr_ref>> const& concrete_address_submap, relevant_memlabels_t const& relevant_memlabels, aliasing_constraints_t const& alias_cons, T_PRECOND_TFG const &src_tfg)
{
  autostop_timer func_timer(__func__);
  fo << src_tfg.graph_to_string() << endl;
  size_t pred_num = 0;
  context *ctx = src->get_context();
  ctx->submap_to_stream(fo, concrete_address_submap);
  //mlasserts->memlabel_assertions_to_stream(fo);
  relevant_memlabels.relevant_memlabels_to_stream(fo, "");

  for (auto const&l : lhs_set) {
    stringstream ss;
    ss << "=lhs-guarded-pred" << pred_num;
    fo << ss.str() << endl;;
    pred_num++;
    guarded_pred_to_stream(fo, l, ss.str());
    //fo << l->to_string_for_eq(true) << endl;
  }
  fo << "=precond" << endl;
  fo << precond.precond_to_string_for_eq(true) << '\n';
  fo << "=edgeguard" << endl;
  ASSERT(eg);
  eg->graph_edge_composition_to_stream(fo, "=edgeguard");

  fo << "=sprel_map_pair" << endl;
  fo << sprel_map_pair.to_string_for_eq();
  fo << "=src_suffixpath" << endl;
  src_suffixpath->graph_edge_composition_to_stream(fo, "=src_suffixpath");
  fo << "=src_avail_exprs" << endl;
  src_avail_exprs.avail_exprs_to_stream(fo, src_tfg.get_locid2expr_map());
  fo << "=aliasing_constraints" << endl;
  fo << alias_cons.to_string_for_eq();
  fo << "=memlabel_map" << endl;
  fo << memlabel_map_to_string(memlabel_map, "");
  fo << "=src" << endl;
  fo << ctx->expr_to_string_table(src, true) << endl;
  fo << "=dst" << endl;
  fo << ctx->expr_to_string_table(dst, true) << endl;
}

}
