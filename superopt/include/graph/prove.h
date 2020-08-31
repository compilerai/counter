#pragma once

#include <list>
#include <fstream>

#include "expr/expr.h"
#include "expr/context.h"
#include "expr/aliasing_constraints.h"
//#include "tfg/predicate.h"
//#include "tfg/predicate_set.h"
#include "tfg/avail_exprs.h"
#include "graph/query_decomposition.h"
#include "support/utils.h"
#include "expr/sprel_map_pair.h"
#include "support/globals_cpp.h"

namespace eqspace {


//expr_ref sprel_map_pair_and_var_locs(sprel_map_pair_t const& sprel_map_pair, expr_ref const &in);
//bool lhs_set_includes_nonstack_mem_equality(list<predicate_ref> const &lhs_set, memlabel_assertions_t const& mlasserts, map<expr_id_t, expr_ref> &input_mem_exprs);


//expr_ref avail_exprs_and(map<graph_loc_id_t, expr_ref> const &avail_exprs, expr_ref const &in, tfg const &t);
//bool expr_set_lhs_prove_src_dst(context *ctx, unordered_set<predicate_ref>const &lhs, edge_guard_t const &guard, expr_ref src, expr_ref dst, query_comment const &qc, bool timeout_res, counter_example_t &counter_example, consts_struct_t const &cs);

//void update_simplify_cache(context *ctx, expr_ref const &a, expr_ref const &b, list<predicate_ref> const &lhs);
//expr_ref get_simplified_expr_using_simplify_cache(context *ctx, expr_ref const &a, list<predicate_ref> const &lhs);
//bool evaluate_counter_example_on_prove_query(context *ctx, list<predicate_ref> const &lhs, expr_ref const &src, expr_ref const &dst, counter_example_t &counter_example);
//bool expr_triple_set_contains(list<predicate_ref> const &haystack, list<predicate_ref> const &needle);

//unordered_set<predicate_ref>local_sprel_expr_guesses_get_pred_set(local_sprel_expr_guesses_t const &g, context* ctx);

//map<expr_id_t, pair<expr_ref, expr_ref>> lhs_set_get_submap(unordered_set<predicate_ref>const &lhs_set, precond_t const &precond, context *ctx);


template<typename T_PRECOND_TFG>
solver::solver_res
is_expr_equal_using_lhs_set_and_precond_helper(context *ctx, list<predicate_ref> const &lhs_set, precond_t const &precond, sprel_map_pair_t const &sprel_map_pair, tfg_suffixpath_t const &src_suffixpath, pred_avail_exprs_t const &src_pred_avail_exprs, graph_memlabel_map_t const &memlabel_map, /*rodata_map_t const &rodata_map,*/ expr_ref const &src, expr_ref const &dst, const query_comment& qc, bool timeout_res, list<counter_example_t> &counter_examples, memlabel_assertions_t const& mlasserts/*set<cs_addr_ref_id_t> const &relevant_addr_refs, vector<memlabel_ref> const& relevant_memlabels*/, aliasing_constraints_t const& alias_cons, T_PRECOND_TFG const &src_tfg, consts_struct_t const &cs)
{
  autostop_timer ft(__func__);

  CPP_DBG_EXEC(EXPR_ID_DETERMINISM, cout << __func__ << " " << __LINE__ << ": src->get_id() = " << src->get_id() << endl);
  CPP_DBG_EXEC(EXPR_ID_DETERMINISM, cout << __func__ << " " << __LINE__ << ": dst->get_id() = " << dst->get_id() << endl);

  graph_symbol_map_t const &symbol_map = src_tfg.get_symbol_map();
  graph_locals_map_t const &locals_map = src_tfg.get_locals_map();

  DYN_DEBUG3(houdini, cout << __func__ << " " << __LINE__ << ": src = " << expr_string(src) << endl);
  DYN_DEBUG3(houdini, cout << __func__ << " " << __LINE__ << ": dst = " << expr_string(dst) << endl);
  //if (is_expr_equal_syntactic_using_lhs_set_and_precond(src, dst, lhs_set, precond, sprel_map_pair, src_suffixpath, src_pred_avail_exprs)) {
  //  CPP_DBG_EXEC2(HOUDINI, cout << __func__ << " " << __LINE__ << ": returning true because is_expr_equal_syntactic_using_lhs_set(src, dst)." << endl);
  //  return solver::solver_res_true;
  //}

  expr_ref rhs = ctx->mk_eq(src, dst);

  autostop_timer func_timer(string(__func__) + ".syntactic_check_failed");
  DYN_DEBUG(counters_enable, stats::get().inc_counter(string(__func__) + ".queries.total"));

  //expr_query_cache_t<pair<bool, counter_example_t>> &cache = ctx->m_cache->m_is_expr_equal_using_lhs_set;
  //if (!ctx->disable_caches()) {
  //  bool lb_found, ub_found;
  //  pair<bool, counter_example_t> lb = make_pair(false, counter_example_t(ctx));
  //  pair<bool, counter_example_t> ub = make_pair(false, counter_example_t(ctx));
  //  cache.find_bounds(lhs_set, precond, sprel_map_pair, src_suffixpath, src_pred_avail_exprs, rhs, lb_found, lb, ub_found, ub);
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

  autostop_timer func_timer2(string(__func__) + "_miss");

  DYN_DEBUG2(houdini, cout << __func__ << " " << __LINE__ << ": rhs = " << expr_string(rhs) << endl);
  expr_ref simplified_rhs = rhs; //expr_do_simplify_using_lhs_set_and_precond(rhs, true, lhs_set, precond, sprel_map_pair, src_suffixpath, src_pred_avail_exprs);
  DYN_DEBUG2(houdini, cout << __func__ << " " << __LINE__ << ": after simplify, rhs = " << expr_string(simplified_rhs) << endl);

  autostop_timer func_timer3(string(__func__) + "_miss_after_simplify");
  expr_ref precond_expr = precond.precond_get_expr(src_tfg, true);
  CPP_DBG_EXEC(EXPR_ID_DETERMINISM, cout << __func__ << " " << __LINE__ << ": precond_expr->get_id() = " << precond_expr->get_id() << endl);
  //precond_expr = ctx->expr_substitute_using_sprel_map_pair(precond_expr, sprel_map_pair);
  //precond_expr = ctx->expr_simplify_using_sprel_pair_and_memlabel_maps(precond_expr, sprel_map_pair, memlabel_map/*, relevant_addr_refs, symbol_map, locals_map, src_tfg.get_argument_regs()*/);
  //map<expr_id_t, pair<expr_ref, expr_ref>> const &rodata_submap = rodata_map.rodata_map_get_submap(ctx, src_tfg);
  //precond_expr = ctx->expr_substitute(precond_expr, rodata_submap);
  CPP_DBG_EXEC(EXPR_ID_DETERMINISM, cout << __func__ << " " << __LINE__ << ": precond_expr->get_id() = " << precond_expr->get_id() << endl);
  precond_expr = ctx->expr_eliminate_constructs_that_the_solver_cannot_handle(precond_expr);
  CPP_DBG_EXEC(EXPR_ID_DETERMINISM, cout << __func__ << " " << __LINE__ << ": precond_expr->get_id() = " << precond_expr->get_id() << endl);
  DYN_DEBUG2(houdini, cout << __func__ << " " << __LINE__ << ": precond = " << precond.precond_to_string() << endl);
  DYN_DEBUG2(houdini, cout << __func__ << " " << __LINE__ << ": before simplify: precond_expr = " << expr_string(precond_expr) << endl);
  //precond_expr = expr_do_simplify_using_lhs_set_and_precond(precond_expr, true, lhs_set, precond, sprel_map_pair, src_suffixpath, src_pred_avail_exprs);
  CPP_DBG_EXEC(EXPR_ID_DETERMINISM, cout << __func__ << " " << __LINE__ << ": precond_expr->get_id() = " << precond_expr->get_id() << endl);
  DYN_DEBUG2(houdini, cout << __func__ << " " << __LINE__ << ": after simplify using lhs set and precond: precond_expr = " << expr_string(precond_expr) << endl);
  if (is_expr_equal_syntactic(precond_expr, expr_false(ctx))) {
    return solver::solver_res_true;
  }

  autostop_timer func_timer10(string(__func__) + "_miss_after_precond_is_false");
  expr_ref pred_set_expr = predicate_set_and(lhs_set, expr_true(ctx), precond, /*rodata_submap,*/ src_tfg);
  DYN_DEBUG4(houdini, cout << __func__ << " " << __LINE__ << ": lhs_expr:\n " << expr_string(pred_set_expr) << endl);
  CPP_DBG_EXEC(EXPR_ID_DETERMINISM, cout << __func__ << " " << __LINE__ << ": pred_set_expr->get_id() = " << pred_set_expr->get_id() << endl);

  //unsubstituted edgecond works for SSA representation (which we assume for src_tfg).
  //substituted edgecond would create long expressions; unsubstituted edge conds are always correct in SSA but may not be useful. Sometimes they are useful, and so let's use them in our lhs set
  expr_ref suffixpath_expr = src_tfg.tfg_suffixpath_get_expr(src_suffixpath);
  CPP_DBG_EXEC(EXPR_ID_DETERMINISM, cout << __func__ << " " << __LINE__ << ": suffixpath_expr->get_id() = " << suffixpath_expr->get_id() << endl);
  suffixpath_expr = ctx->expr_eliminate_constructs_that_the_solver_cannot_handle(suffixpath_expr);
  CPP_DBG_EXEC(EXPR_ID_DETERMINISM, cout << __func__ << " " << __LINE__ << ": suffixpath_expr->get_id() = " << suffixpath_expr->get_id() << endl);

  expr_ref pred_avail_expr = pred_avail_exprs_and(src_pred_avail_exprs, expr_true(ctx), src_tfg, src_suffixpath);
  CPP_DBG_EXEC(EXPR_ID_DETERMINISM, cout << __func__ << " " << __LINE__ << ": pred_avail_expr->get_id() = " << pred_avail_expr->get_id() << endl);
  pred_avail_expr = ctx->expr_eliminate_constructs_that_the_solver_cannot_handle(pred_avail_expr);
  CPP_DBG_EXEC(EXPR_ID_DETERMINISM, cout << __func__ << " " << __LINE__ << ": pred_avail_expr->get_id() = " << pred_avail_expr->get_id() << endl);

  expr_ref sprel_map_pair_expr = sprel_map_pair_and_var_locs(sprel_map_pair, expr_true(ctx)); // in prove, we add the non-var locs
  CPP_DBG_EXEC(EXPR_ID_DETERMINISM, cout << __func__ << " " << __LINE__ << ": sprel_map_pair_expr->get_id() = " << sprel_map_pair_expr->get_id() << endl);
  CPP_DBG_EXEC(EXPR_ID_DETERMINISM, cout << __func__ << " " << __LINE__ << ": sprel_map_pair_expr = " << ctx->expr_to_string_table(sprel_map_pair_expr, true) << endl);

  // -- all simplifications should have been performed before this --

  expr_ref lhs_expr = pred_set_expr;
  CPP_DBG_EXEC(EXPR_ID_DETERMINISM, cout << __func__ << " " << __LINE__ << ": lhs_expr->get_id() = " << lhs_expr->get_id() << endl);
  lhs_expr = expr_and(precond_expr, lhs_expr);
  CPP_DBG_EXEC(EXPR_ID_DETERMINISM, cout << __func__ << " " << __LINE__ << ": lhs_expr->get_id() = " << lhs_expr->get_id() << endl);
  lhs_expr = expr_and(suffixpath_expr, lhs_expr);
  CPP_DBG_EXEC(EXPR_ID_DETERMINISM, cout << __func__ << " " << __LINE__ << ": lhs_expr->get_id() = " << lhs_expr->get_id() << endl);
  lhs_expr = expr_and(pred_avail_expr, lhs_expr);
  CPP_DBG_EXEC(EXPR_ID_DETERMINISM, cout << __func__ << " " << __LINE__ << ": lhs_expr->get_id() = " << lhs_expr->get_id() << endl);
  lhs_expr = expr_and(sprel_map_pair_expr, lhs_expr);

  autostop_timer func_timer4(string(__func__) + "_miss_after_simplify_and_add_auxiliary_structures");
  DYN_DEBUG4(houdini, cout << __func__ << " " << __LINE__ << ": lhs_expr:\n" << expr_string(lhs_expr) << endl);
  CPP_DBG_EXEC(EXPR_ID_DETERMINISM, cout << __func__ << " " << __LINE__ << ": lhs_expr->get_id() = " << lhs_expr->get_id() << endl);

  DYN_DEBUG4(houdini, cout << __func__ << " " << __LINE__ << ": aliasing constraints =\n" << alias_cons.to_string_for_eq() << endl);
  expr_ref alias_cons_expr = alias_cons.convert_to_expr(ctx);
  DYN_DEBUG4(houdini, cout << __func__ << " " << __LINE__ << ": aliasing constraints expr = " << expr_string(alias_cons_expr) << endl);

  CPP_DBG_EXEC(EXPR_ID_DETERMINISM, cout << __func__ << " " << __LINE__ << ": aliasing_cons_expr  =\n" << ctx->expr_to_string_table(alias_cons_expr, true) << endl);

  lhs_expr = expr_and(alias_cons_expr, lhs_expr);
  CPP_DBG_EXEC(EXPR_ID_DETERMINISM, cout << __func__ << " " << __LINE__ << ": lhs_expr->get_id() = " << lhs_expr->get_id() << endl);

  map<expr_id_t,pair<expr_ref,expr_ref>> const& memlabel_range_submap = mlasserts.memlabel_assertions_get_submap();
  lhs_expr = ctx->expr_substitute(lhs_expr, memlabel_range_submap);
  CPP_DBG_EXEC(EXPR_ID_DETERMINISM, cout << __func__ << " " << __LINE__ << ": lhs_expr->get_id() = " << lhs_expr->get_id() << endl);
  simplified_rhs = ctx->expr_substitute(simplified_rhs, memlabel_range_submap);
  CPP_DBG_EXEC(EXPR_ID_DETERMINISM, cout << __func__ << " " << __LINE__ << ": simplified_rhs->get_id() = " << simplified_rhs->get_id() << endl);
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

  expr_ref memlabel_rangeval_assertions = mlasserts.get_assertion();
  //if (memlabel_range_submap.empty()) {
  //  memlabel_rangeval_assertions = generate_range_assertions_for_memlabels(ctx, relevant_memlabels, rodata_map);
  //  CPP_DBG_EXEC3(HOUDINI, cout << __func__ << " " << __LINE__ << ": memlabel_rangeval_assertions = " << ctx->expr_to_string_table(memlabel_rangeval_assertions) << endl);
  //} else {
  //  memlabel_rangeval_assertions = ctx->submap_generate_assertions(memlabel_range_submap);
  //}
  //ASSERT(memlabel_rangeval_assertions);
  CPP_DBG_EXEC(EXPR_ID_DETERMINISM, cout << __func__ << " " << __LINE__ << ": memlabel_rangeval_assertions->get_id() = " << memlabel_rangeval_assertions->get_id() << endl);

  //lhs_expr = expr_and(memlabel_assertions, lhs_expr);
  lhs_expr = expr_and(memlabel_rangeval_assertions, lhs_expr);

  map<expr_id_t, expr_ref> input_mem_exprs;
  expr_ref nonstack_memvar;
  if (   !ctx->expr_has_stack_and_nonstack_memlabels_occuring_together(lhs_expr)
      && !ctx->expr_has_stack_and_nonstack_memlabels_occuring_together(simplified_rhs)) {
    CPP_DBG_EXEC(EXPR_ID_DETERMINISM, cout << __func__ << " " << __LINE__ << ": lhs_expr->get_id() = " << lhs_expr->get_id() << endl);
    lhs_expr = ctx->expr_model_stack_as_separate_mem(lhs_expr);
    CPP_DBG_EXEC(EXPR_ID_DETERMINISM, cout << __func__ << " " << __LINE__ << ": lhs_expr->get_id() = " << lhs_expr->get_id() << endl);
    simplified_rhs = ctx->expr_model_stack_as_separate_mem(simplified_rhs);
    CPP_DBG_EXEC(EXPR_ID_DETERMINISM, cout << __func__ << " " << __LINE__ << ": simplified_rhs->get_id() = " << simplified_rhs->get_id() << endl);
    if (lhs_set_includes_nonstack_mem_equality(lhs_set, mlasserts, input_mem_exprs)) {
      ASSERT(input_mem_exprs.size());
      nonstack_memvar = ctx->mk_var(G_SOLVER_NONSTACK_MEM_NAME, input_mem_exprs.begin()->second->get_sort());
      lhs_expr = ctx->expr_replace_input_memvars_with_nonstack_memvar(lhs_expr, input_mem_exprs, nonstack_memvar);
      simplified_rhs = ctx->expr_replace_input_memvars_with_nonstack_memvar(simplified_rhs, input_mem_exprs, nonstack_memvar);
    }
  } else {
    NOT_REACHED();  //for now, we expect inputs where this is not possible
  }

  DYN_DEBUG4(houdini, cout << __func__ << " " << __LINE__ << ": lhs_expr = " << expr_string(lhs_expr) << endl);
  expr_ref e = expr_implies(lhs_expr, simplified_rhs);
  DYN_DEBUG4(houdini, cout << __func__ << " " << __LINE__ << ": e = " << expr_string(e) << endl);

  autostop_timer func_timer9(string(__func__) + "_miss_after_aliasing_constraints_generation");

  DYN_DEBUG2(smt_query,
    stringstream ss;

    string pid = int_to_string(getpid());
    ss << g_query_dir << "/" << IS_EXPR_EQUAL_USING_LHS_SET_FILENAME_PREFIX << "." << qc.to_string();
    string filename = ss.str();
    std::ofstream fo;
    fo.open(filename.data());
    ASSERT(fo);
    output_lhs_set_precond_and_src_dst_to_file(fo, lhs_set, precond, sprel_map_pair, src_suffixpath, src_pred_avail_exprs, /*rodata_map,*/ alias_cons, src, dst, src_tfg);
    fo.close();
    //g_query_files.insert(filename);
    //expr_num = (expr_num + 1) % 2; //don't allow more than 500 files per process
    cout << "is_expr_equal_using_lhs_set filename " << filename << endl;
  );

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
      cache.add(lhs_set, precond, sprel_map_pair, src_suffixpath, src_pred_avail_exprs, rhs, make_pair(false, counter_example));
    }
    return solver::solver_res_false;
  }*/

  //autostop_timer func_timer6(string(__func__) + "_miss_after_checking_counter_example_exists");
  set<memlabel_ref> const& relevant_memlabels = mlasserts.get_relevant_memlabels();

  solver::solver_res ret = ctx->expr_is_provable(e, qc, relevant_memlabels, counter_examples);

  autostop_timer func_timer7(string(__func__) + "_miss_after_expr_is_provable");

  if (ret == solver::solver_res_false) {
    for (auto& ce : counter_examples) {
      if (!ce.is_empty()) {
        if (input_mem_exprs.size()) {
          ASSERT(nonstack_memvar);
          ce.add_input_memvars_using_nonstack_memvar(input_mem_exprs, nonstack_memvar);
        }
        ce.reconciliate_stack_mem_in_counter_example();
        //alignment_map_t const& align_map = create_alignment_map_from_predicate_set(cs, lhs_set, symbol_map, locals_map);
        //ce.add_missing_memlabel_lb_and_ub_consts(ctx/*, alias_cons*/, relevant_memlabels, align_map);
      }
    }
  }

  return ret;
}

template<typename T_PRECOND_TFG>
solver::solver_res
is_expr_equal_using_lhs_set_and_precond_with_query_decomposition(context *ctx, list<predicate_ref> const &lhs_set, precond_t const &precond, sprel_map_pair_t const &sprel_map_pair, tfg_suffixpath_t const &src_suffixpath, pred_avail_exprs_t const &src_pred_avail_exprs, graph_memlabel_map_t const &memlabel_map, /*rodata_map_t const &rodata_map,*/ expr_ref const &src, expr_ref const &dst, const query_comment& qc, bool timeout_res, list<counter_example_t> &counter_examples, memlabel_assertions_t const& mlasserts/*, set<cs_addr_ref_id_t> const &relevant_addr_refs, vector<memlabel_ref> const& relevant_memlabels*/, aliasing_constraints_t const& rhs_alias_cons, T_PRECOND_TFG const &src_tfg, consts_struct_t const &cs)
{
  // XXX pass a new counter example object to query decomposition as it doesn't seem to return a valid counter example
  list<counter_example_t> unused_counter_examples;

  // generate precond expr and simplify it before passing it query decomposition
  expr_ref precond_expr = precond.precond_get_expr(src_tfg, true);
  precond_expr = ctx->expr_eliminate_constructs_that_the_solver_cannot_handle(precond_expr);
  DYN_DEBUG2(houdini, cout << __func__ << " " << __LINE__ << ": before simplify: precond_expr = " << expr_string(precond_expr) << endl);
  precond_expr = ctx->expr_do_simplify(precond_expr);
  DYN_DEBUG2(houdini, cout << __func__ << " " << __LINE__ << ": after simplify: precond_expr = " << expr_string(precond_expr) << endl);

  query_decomposer obj(precond_expr, src, dst);
  if (!obj.find_mappings(lhs_set, precond, sprel_map_pair, src_suffixpath, src_pred_avail_exprs, memlabel_map, qc, timeout_res, unused_counter_examples, mlasserts, rhs_alias_cons, src_tfg, cs)) {
    // return timeout if query-decomposition couldn't prove equality
    return solver::solver_res_timeout;
  }

  return solver::solver_res_true;
}

template<typename T_PRECOND_TFG>
bool
is_expr_equal_using_lhs_set_and_precond(context *ctx, list<predicate_ref> const &lhs_set, precond_t const &precond, sprel_map_pair_t const &sprel_map_pair, tfg_suffixpath_t const &src_suffixpath, pred_avail_exprs_t const &src_pred_avail_exprs, graph_memlabel_map_t const &memlabel_map, /*rodata_map_t const &rodata_map,*/ expr_ref const &src, expr_ref const &dst, const query_comment& qc, bool timeout_res, list<counter_example_t> &counter_examples, memlabel_assertions_t const& mlasserts/*, set<cs_addr_ref_id_t> const &relevant_addr_refs, vector<memlabel_ref> const& relevant_memlabels*/, aliasing_constraints_t const& rhs_alias_cons, T_PRECOND_TFG const &src_tfg, consts_struct_t const &cs)
{
  solver::solver_res ret = is_expr_equal_using_lhs_set_and_precond_helper(ctx, lhs_set, precond, sprel_map_pair, src_suffixpath, src_pred_avail_exprs, memlabel_map, /*rodata_map,*/ src, dst, qc, timeout_res, counter_examples, mlasserts/*relevant_addr_refs, relevant_memlabels*/, rhs_alias_cons, src_tfg, cs);
  if (ret == solver::solver_res_timeout) {
    if(!ctx->get_config().disable_query_decomposition)
    {
      stats::get().inc_counter("query-decomposition-invoked");
      ret = is_expr_equal_using_lhs_set_and_precond_with_query_decomposition(ctx, lhs_set, precond, sprel_map_pair, src_suffixpath, src_pred_avail_exprs, memlabel_map, /*rodata_map,*/ src, dst, qc, timeout_res, counter_examples, mlasserts/*relevant_addr_refs, relevant_memlabels*/, rhs_alias_cons, src_tfg, cs);
      if (ret == solver::solver_res_timeout) {
        stats::get().inc_counter("query-decomposition-timed-out");
      }
      CPP_DBG_EXEC(GENERAL, cout << __func__ << " " << __LINE__ << " Result of Query Decomposition is: " << solver::solver_res_to_string(ret) << endl);
    }
  }
  if (ret == solver::solver_res_timeout) {
    return timeout_res;
  } else {
    return ret == solver::solver_res_true;
  }
}

template<typename T_PRECOND_TFG>
bool
prove(context *ctx, predicate_set_t const &lhs, precond_t const &precond, sprel_map_pair_t const &sprel_map_pair, tfg_suffixpath_t const &src_suffixpath, pred_avail_exprs_t const &src_pred_avail_exprs, /*rodata_map_t const &rodata_map,*/ graph_memlabel_map_t const &memlabel_map, expr_ref const &src, expr_ref const &dst, query_comment const &comment, bool timeout_res, graph_symbol_map_t const &symbol_map, graph_locals_map_t const &locals_map, memlabel_assertions_t const& mlasserts/*set<cs_addr_ref_id_t> const &relevant_addr_refs, vector<memlabel_ref> const& relevant_memlabels*/, aliasing_constraints_t const& alias_cons, T_PRECOND_TFG const &src_tfg, consts_struct_t const &cs, list<counter_example_t> &counter_examples)
{
  autostop_timer func_timer(__func__);
  ctx->increment_num_proof_queries();

  list<predicate_ref> lhs_set1;
  expr_ref src_simplified, dst_simplified;

  lhs_set1 = predicate::determinized_pred_set(lhs);
  src_simplified = src;
  dst_simplified = dst;

  //CAUTION: any changes in this simplification/substitution also need to be repeated at the time of precond_expr computation in is_expr_equal_using_lhs_set_and_precond_helper

  DYN_DEBUG3(houdini, cout << __func__ << " " << __LINE__ << ":  before is_expr_equal_syntactic check, src_simplified: " << expr_string(src_simplified) << '\n');
  DYN_DEBUG3(houdini, cout << __func__ << " " << __LINE__ << ":  before is_expr_equal_syntactic check, dst_simplified: " << expr_string(dst_simplified) << '\n');

  if (is_expr_equal_syntactic(src_simplified, dst_simplified)) {
    ctx->increment_num_proof_queries_answered_by_syntactic_check();
    DYN_DEBUG2(houdini, cout << __func__ << " " << __LINE__ << ": returning true because is_expr_equal_syntactic(src, dst)\n");
    return true;
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

  list<predicate_ref> sprel_preds = sprel_map_pair.get_predicates_for_non_var_locs(ctx);
  list_append(lhs_set1, sprel_preds); // Later, in is_expr_equal_syntactic_using_lhs_set_and_precond, we add the var locs as well

  DYN_DEBUG4(houdini, cout << __func__ << " " << __LINE__ << ": before expr_eliminate_constructs_that_the_solver_cannot_handle, src_simplified: " << expr_string(src_simplified) << endl);
  DYN_DEBUG4(houdini, cout << __func__ << " " << __LINE__ << ": before expr_eliminate_constructs_that_the_solver_cannot_handle, dst_simplified: " << expr_string(dst_simplified) << endl);

  // we somehow need to call eliminate_constructs_that_solver_cannot_handle twice; needs investigation
  stats::get().get_timer(string(__func__) + ".eliminate_constructs_that_the_solver_cannot_handle1")->start();
  lhs_set1 = lhs_set_eliminate_constructs_that_the_solver_cannot_handle(lhs_set1, ctx);
  src_simplified = ctx->expr_eliminate_constructs_that_the_solver_cannot_handle(src_simplified);
  dst_simplified = ctx->expr_eliminate_constructs_that_the_solver_cannot_handle(dst_simplified);
  stats::get().get_timer(string(__func__) + ".eliminate_constructs_that_the_solver_cannot_handle1")->stop();

  DYN_DEBUG4(houdini, cout << __func__ << " " << __LINE__ << ":  after simplification, src_simplified: " << expr_string(src_simplified) << '\n');
  DYN_DEBUG4(houdini, cout << __func__ << " " << __LINE__ << ":  after simplification, dst_simplified: " << expr_string(dst_simplified) << '\n');

  if (is_expr_equal_syntactic(src_simplified, dst_simplified)) {
    ctx->increment_num_proof_queries_answered_by_syntactic_check();
    DYN_DEBUG4(houdini, cout << __func__ << " " << __LINE__ << ": is_expr_equal_syntactic() returned true\n");
    return true;
  }

  DYN_DEBUG4(houdini, cout << __func__ << " " << __LINE__ << ": lhs_set1 =\n"; for (auto p : lhs_set1) cout << __func__ << " " << __LINE__ << ": p = " << p->to_string(true) << endl);

  bool ret = is_expr_equal_using_lhs_set_and_precond(ctx, lhs_set1, precond, sprel_map_pair, src_suffixpath, src_pred_avail_exprs, memlabel_map, /*rodata_map_pruned,*/ src_simplified, dst_simplified, comment, timeout_res, counter_examples, mlasserts/*relevant_addr_refs, relevant_memlabels*/, alias_cons, src_tfg, cs);
  for (auto& ce : counter_examples) {
    //ce.set_rodata_submap(rodata_submap);
    ce.counter_example_set_local_sprel_expr_assumes(precond.get_local_sprel_expr_assumes());
  }

  DYN_DEBUG3(houdini, cout << __func__ << " " << __LINE__ << ": returning " << ret << endl);
  return ret;
}

template<typename T_PRECOND_TFG>
void
output_lhs_set_precond_and_src_dst_to_file(ostream &fo, list<predicate_ref> const &lhs_set, precond_t const &precond, sprel_map_pair_t const &sprel_map_pair, shared_ptr<tfg_edge_composition_t> const &src_suffixpath, pred_avail_exprs_t const &src_pred_avail_exprs, /*rodata_map_t const &rodata_map,*/ aliasing_constraints_t const& alias_cons, expr_ref const &src, expr_ref const &dst, T_PRECOND_TFG const &src_tfg)
{
  autostop_timer func_timer(__func__);
  fo << src_tfg.graph_to_string() << endl;
  size_t pred_num = 0;
  context *ctx = src->get_context();
  for (auto l : lhs_set) {
    fo << "=lhs-pred" << pred_num << endl;
    pred_num++;
    fo << l->to_string_for_eq(true) << endl;
  }
  fo << "=edgeguard" << endl;
  fo << precond.precond_get_guard().edge_guard_to_string_for_eq() << endl;
  fo << "=lsprels" << endl;
  fo << precond.get_local_sprel_expr_assumes().to_string_for_eq(true) << endl;
  fo << "=sprel_map_pair" << endl;
  fo << sprel_map_pair.to_string_for_eq();
  fo << "=src_suffixpath" << endl;
  //fo << (src_suffixpath ? src_suffixpath->graph_edge_composition_to_string() : "(" EPSILON ")") << endl;
  fo << src_suffixpath->graph_edge_composition_to_string_for_eq("=src_suffixpath");
  fo << "=src_pred_avail_exprs" << endl;
  fo << pred_avail_exprs_to_string(src_pred_avail_exprs);
  //fo << "=rodata_map" << endl;
  //fo << rodata_map.to_string_for_eq();
  fo << "=aliasing_constraints" << endl;
  fo << alias_cons.to_string_for_eq();
  fo << "=src" << endl;
  fo << ctx->expr_to_string_table(src, true) << endl;
  fo << "=dst" << endl;
  fo << ctx->expr_to_string_table(dst, true) << endl;
}

template<typename T_PRECOND_TFG>
void
output_lhs_set_guard_lsprels_and_src_dst_to_file(ostream &fo, predicate_set_t const &lhs_set, precond_t const& precond/*edge_guard_t const &guard*/, local_sprel_expr_guesses_t const &local_sprel_expr_assumes_required_to_prove, sprel_map_pair_t const &sprel_map_pair, shared_ptr<tfg_edge_composition_t> const &src_suffixpath, pred_avail_exprs_t const &src_pred_avail_exprs, /*rodata_map_t const &rodata_map,*/ aliasing_constraints_t const& alias_cons, graph_memlabel_map_t const &/*src_*/memlabel_map/*, graph_memlabel_map_t const &dst_memlabel_map*/, set<local_sprel_expr_guesses_t> const &all_guesses, expr_ref src, expr_ref dst, memlabel_assertions_t const& mlasserts/*, set<cs_addr_ref_id_t> const &relevant_addr_refs*/, T_PRECOND_TFG const &src_tfg)
{
  autostop_timer func_timer(__func__);
  //edge_guard_t const &guard = precond.precond_get_guard();
  //local_sprel_expr_guesses_t const &local_sprel_expr_assumes_required_to_prove = precond.get_local_sprel_expr_assumes();
  fo << src_tfg.graph_to_string() << endl;
  size_t pred_num = 0;
  context *ctx = src->get_context();
  consts_struct_t const &cs = ctx->get_consts_struct();
  //fo << cs.relevant_addr_refs_to_string(relevant_addr_refs) << endl;
  mlasserts.memlabel_assertions_to_stream(fo);
  for (auto const&l : lhs_set) {
    fo << "=lhs-pred" << pred_num << endl;
    pred_num++;
    fo << l->to_string_for_eq(true) << endl;
  }
  fo << "=precond" << endl;
  fo << precond.precond_to_string_for_eq(true);
  fo << "=local_sprel_expr_assumes_required_to_prove" << endl;
  fo << local_sprel_expr_assumes_required_to_prove.to_string_for_eq(true) << endl;
  fo << "=sprel_map_pair" << endl;
  fo << sprel_map_pair.to_string_for_eq();
  fo << "=src_suffixpath" << endl;
  //fo << src_suffixpath->graph_edge_composition_to_string() << endl;
  fo << src_suffixpath->graph_edge_composition_to_string_for_eq("=src_suffixpath");
  fo << "=src_pred_avail_exprs" << endl;
  fo << pred_avail_exprs_to_string(src_pred_avail_exprs);
  //fo << "=rodata_map" << endl;
  //fo << rodata_map.to_string_for_eq();
  fo << "=aliasing_constraints" << endl;
  fo << alias_cons.to_string_for_eq();
  //fo << "=src_memlabel_map" << endl;
  //fo << memlabel_map_to_string(src_memlabel_map);
  //fo << "=dst_memlabel_map" << endl;
  //fo << memlabel_map_to_string(dst_memlabel_map);
  fo << "=memlabel_map" << endl;
  fo << memlabel_map_to_string(memlabel_map);
  //fo << "=memlabel_assertions\n";
  //ctx->get_memlabel_assertions().memlabel_assertions_to_stream(fo);
  fo << "=all_guesses.num" << all_guesses.size() << endl;
  size_t i = 0;
  for (auto g : all_guesses) {
    fo << "=guess" << i << endl;
    fo << g.to_string_for_eq(true) << endl;
    i++;
  }
  fo << "=src" << endl;
  fo << ctx->expr_to_string_table(src, true) << endl;
  fo << "=dst" << endl;
  fo << ctx->expr_to_string_table(dst, true) << endl;
}




}
