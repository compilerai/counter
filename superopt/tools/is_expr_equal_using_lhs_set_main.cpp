#include <atomic>
#include <fstream>
#include <iostream>
#include <string>

#include "eq/eqcheck.h"
#include "eq/parse_input_eq_file.h"
#include "expr/consts_struct.h"
#include "support/mytimer.h"
#include "support/log.h"
#include "support/cl.h"
#include "support/globals.h"
#include "expr/expr.h"
#include "expr/local_sprel_expr_guesses.h"
#include "expr/aliasing_constraints.h"
#include "expr/z3_solver.h"
#include "support/timers.h"


#define DEBUG_TYPE "main"

using namespace std;

int main(int argc, char **argv)
{
  // command line args processing
  cl::arg<string> expr_file(cl::implicit_prefix, "", "path to .expr file");
  cl::arg<unsigned> smt_query_timeout(cl::explicit_prefix, "smt-query-timeout", 600, "Timeout per query (s)");
  cl::arg<unsigned> sage_query_timeout(cl::explicit_prefix, "sage-query-timeout", 600, "Timeout per query (s)");
  cl::arg<string> debug(cl::explicit_prefix, "dyn-debug", "", "Enable dynamic debugging for debug-class(es).  Expects comma-separated list of debug-classes with optional level e.g. --debug=correlate=2,smt_query=1");
  cl::arg<bool> syntactic(cl::explicit_flag, "syntactic", false, "Use syntactic check only");
  cl::arg<bool> simplify(cl::explicit_flag, "simplify", false, "Only simplify the input file and output it");
  cl::arg<bool> ignore_all_undef_behaviour_lhs_preds(cl::explicit_flag, "ignore-all-undef-behaviour-lhs-preds", false, "Remove all undef behaviour lhs preds and output the revised query");
  cl::arg<bool> ignore_all_sprel_assume_lhs_preds(cl::explicit_flag, "ignore-all-sprel-assume-lhs-preds", false, "Remove all sprel assume lhs preds and output the revised query");
  cl::cl cmd("Checks if two expressions are equal under preconditions (lhs set)");
  cmd.add_arg(&expr_file);
  cmd.add_arg(&smt_query_timeout);
  cmd.add_arg(&sage_query_timeout);
  cmd.add_arg(&debug);
  cmd.add_arg(&syntactic);
  cmd.add_arg(&simplify);
  cmd.add_arg(&ignore_all_undef_behaviour_lhs_preds);
  cmd.add_arg(&ignore_all_sprel_assume_lhs_preds);
  cmd.parse(argc, argv);

  init_dyn_debug_from_string(debug.get_value());

  DYN_DBG_ELEVATE(prove_path, 1);
  DYN_DBG_ELEVATE(smt_query_debug, 1);

  CPP_DBG_EXEC(DYN_DEBUG, print_debug_class_levels());

  context::config cfg(smt_query_timeout.get_value(), sage_query_timeout.get_value());
  context ctx(cfg);
  ctx.parse_consts_db(CONSTS_DB_FILENAME);
  consts_struct_t &cs = ctx.get_consts_struct();
  solver_init();
  g_query_dir_init();

  ifstream in(expr_file.get_value());
  if(!in.is_open()) {
    cout << __func__ << " " << __LINE__ << ": could not open " << expr_file.get_value() << "." << endl;
    exit(1);
  }

  list<guarded_pred_t> lhs_set;
  precond_t precond;
  sprel_map_pair_t sprel_map_pair;
  tfg_suffixpath_t src_suffixpath;
  avail_exprs_t src_avail_exprs;
  aliasing_constraints_t alias_cons;
  graph_memlabel_map_t memlabel_map;
  expr_ref src, dst;
  map<expr_id_t, expr_ref> src_map, dst_map;
  //dshared_ptr<memlabel_assertions_t> mlasserts;
  dshared_ptr<tfg_llvm_t> src_tfg;
  graph_edge_composition_ref<pc, tfg_edge> eg;
  map<expr_id_t, pair<expr_ref, expr_ref>> concrete_address_submap;

  relevant_memlabels_t relevant_memlabels({});

  read_lhs_set_guard_etc_and_src_dst_from_file(in, &ctx, lhs_set, precond, eg, sprel_map_pair, src_suffixpath, src_avail_exprs, alias_cons, memlabel_map, src, dst, src_map, dst_map, concrete_address_submap/*mlasserts*/, src_tfg, relevant_memlabels);

  set<memlabel_ref> const& relevant_memlabels_set = relevant_memlabels.relevant_memlabels_get_ml_set();

  //solver_res ret;
  if (ignore_all_undef_behaviour_lhs_preds.get_value()) {
    list<guarded_pred_t> lhs_set_shortened;
    for (auto const& [g,pred] : lhs_set) {
      if (pred->get_comment().find(UNDEF_BEHAVIOUR_ASSUME_COMMENT_PREFIX) == 0) {
        continue;
      }
      lhs_set_shortened.emplace_back(g,pred);
    }
    output_lhs_set_guard_etc_and_src_dst_to_file(cout, lhs_set, precond, eg, sprel_map_pair, src_suffixpath, src_avail_exprs, memlabel_map, src, dst, concrete_address_submap/*mlasserts*/, relevant_memlabels, alias_cons, *src_tfg);
  } else if (ignore_all_sprel_assume_lhs_preds.get_value()) {
    list<guarded_pred_t> lhs_set_shortened;
    for (auto const& [g,pred] : lhs_set) {
      if (pred->get_comment().find(SPREL_ASSUME_COMMENT_PREFIX) == 0) {
        continue;
      }
      lhs_set_shortened.emplace_back(g,pred);
    }
    output_lhs_set_guard_etc_and_src_dst_to_file(cout, lhs_set, precond, eg, sprel_map_pair, src_suffixpath, src_avail_exprs, memlabel_map, src, dst, concrete_address_submap/*mlasserts*/, relevant_memlabels, alias_cons, *src_tfg);
  //} else if (simplify.get_value()) {
  //  expr_ref src_simpl = expr_simplify::expr_do_simplify_using_lhs_set_and_precond(src, true, {}, precond, sprel_map_pair, graph_symbol_map_t(), graph_locals_map_t());
  //  expr_ref dst_simpl = expr_simplify::expr_do_simplify_using_lhs_set_and_precond(dst, true, {}, precond, sprel_map_pair, graph_symbol_map_t(), graph_locals_map_t());

  //  output_lhs_set_guard_etc_and_src_dst_to_file(cout, lhs_set, precond, eg, sprel_map_pair, src_suffixpath, src_avail_exprs, alias_cons, memlabel_map, src_simpl, dst_simpl, mlasserts, relevant_memlabels, *src_tfg);
  //} else if (syntactic.get_value()) {
  //  ret = expr_simplify::is_expr_equal_syntactic_using_lhs_set_and_precond(src, dst, {}, precond, sprel_map_pair);
  //  cout << ((ret == solver::solver_res_true) ? "EQUAL" : "NOT-EQUAL") << endl;
  } else {
    src_tfg->populate_auxilliary_structures_dependent_on_locs();
    query_comment qc(__func__);
    //list<counter_example_t> ces;
    std::atomic<bool> should_return_sharedvar;
    proof_result_t proof_result = is_expr_equal_using_lhs_set_and_precond(&ctx, lhs_set, precond, eg, sprel_map_pair, src_suffixpath, src_avail_exprs, memlabel_map, src, dst, qc/*, ces*/, concrete_address_submap/*mlasserts*/, relevant_memlabels, alias_cons, *src_tfg, should_return_sharedvar);
    cout << ((proof_result.get_proof_status() == proof_status_proven) ? "EQUAL" : "NOT-EQUAL") << endl;
  }

  solver_kill();
  call_on_exit_function();

  exit(0);
}
