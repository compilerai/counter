#include "eq/eqcheck.h"
#include "tfg/parse_input_eq_file.h"
#include "expr/consts_struct.h"
#include "support/mytimer.h"
#include "support/log.h"
#include "support/cl.h"
#include "support/globals.h"
#include "support/dyn_debug.h"
#include "expr/z3_solver.h"
#include "expr/expr.h"
#include "expr/expr_utils.h"
#include "eq/corr_graph.h"

#include "support/timers.h"

#include <fstream>

#include <iostream>
#include <string>

using namespace std;

int main(int argc, char **argv)
{
  // command line args processing
  cl::arg<string> expr_file(cl::implicit_prefix, "", "path to input file");
  cl::arg<unsigned> smt_query_timeout(cl::explicit_prefix, "smt-query-timeout", 600, "Timeout per query (s)");
  cl::arg<unsigned> sage_query_timeout(cl::explicit_prefix, "sage-query-timeout", 600, "Timeout per query (s)");
  cl::arg<string> debug(cl::explicit_prefix, "debug", "", "Enable dynamic debugging for debug-class(es).  Expects comma-separated list of debug-classes with optional level e.g. --debug=correlate=2,smt_query=1");
  cl::arg<string> eval_expr(cl::explicit_prefix, "eval-expr", "", "Evaluate the returned counter example (if any) on expr in this file");
  //cl::arg<bool> syntactic(cl::explicit_flag, "syntactic", false, "Use syntactic check only");
  //cl::arg<bool> simplify(cl::explicit_flag, "simplify", false, "Only simplify the input file and output it");
  //cl::arg<bool> ignore_all_undef_behaviour_lhs_preds(cl::explicit_flag, "ignore-all-undef-behaviour-lhs-preds", false, "Remove all undef behaviour lhs preds and output the revised query");
  //cl::arg<bool> ignore_all_sprel_assume_lhs_preds(cl::explicit_flag, "ignore-all-sprel-assume-lhs-preds", false, "Remove all sprel assume lhs preds and output the revised query");
  cl::cl cmd("Expression Simplifier using Preconditions (lhs set)");
  cmd.add_arg(&expr_file);
  cmd.add_arg(&smt_query_timeout);
  cmd.add_arg(&sage_query_timeout);
  cmd.add_arg(&eval_expr);
  //cmd.add_arg(&syntactic);
  //cmd.add_arg(&simplify);
  //cmd.add_arg(&ignore_all_undef_behaviour_lhs_preds);
  //cmd.add_arg(&ignore_all_sprel_assume_lhs_preds);
  cmd.add_arg(&debug);
  cmd.parse(argc, argv);

  eqspace::init_dyn_debug_from_string(debug.get_value());
  CPP_DBG_EXEC(DYN_DEBUG, eqspace::print_debug_class_levels());
  context::config cfg(smt_query_timeout.get_value(), sage_query_timeout.get_value());
  context ctx(cfg);
  //consts_struct_t cs;
  //cs.parse_consts_db(&ctx);
  ctx.parse_consts_db(CONSTS_DB_FILENAME);
  consts_struct_t &cs = ctx.get_consts_struct();
  solver_init();

  g_query_dir_init();

  DBG_SET(CE_ADD, 4);
  //DBG_SET(UNALIASED_MEMSLOT, 2);

  ifstream in(expr_file.get_value());
  if(!in.is_open()) {
    cout << __func__ << " " << __LINE__ << ": could not open " << expr_file.get_value() << "." << endl;
    exit(1);
  }

  predicate_set_t lhs_set;
  precond_t precond;
  local_sprel_expr_guesses_t lsprel_guesses;
  sprel_map_pair_t sprel_map_pair;
  tfg_suffixpath_t src_suffixpath;
  pred_avail_exprs_t src_pred_avail_exprs;
  aliasing_constraints_t alias_cons;
  graph_memlabel_map_t memlabel_map;
  set<local_sprel_expr_guesses_t> all_guesses;
  expr_ref src, dst;
  map<expr_id_t, expr_ref> src_map, dst_map;
  shared_ptr<memlabel_assertions_t> mlasserts;
  shared_ptr<tfg> src_tfg;

  read_lhs_set_guard_lsprels_and_src_dst_from_file(in, &ctx, lhs_set, precond, lsprel_guesses, sprel_map_pair, src_suffixpath, src_pred_avail_exprs, alias_cons, memlabel_map, all_guesses, src, dst, src_map, dst_map, mlasserts, src_tfg);

  vector<memlabel_ref> relevant_memlabels;
  memlabel_map_get_relevant_memlabels(memlabel_map, relevant_memlabels);
  set<memlabel_ref> relevant_memlabels_set(relevant_memlabels.begin(), relevant_memlabels.end());

  DBG_SET(MEMLABEL_ASSERTIONS, 2);
  DYN_DBG_SET(smt_query, 2);
  DYN_DBG_SET(smt_file, 1);

  src_tfg->populate_auxilliary_structures_dependent_on_locs();
  src_tfg->populate_simplified_edgecond(nullptr);
  src_tfg->populate_simplified_to_state(nullptr);
  src_tfg->populate_simplified_assumes(nullptr);
  src_tfg->populate_simplified_assert_map(src_tfg->get_all_pcs());

  //predicate pred(precond_t(&ctx), src, dst, "debug", predicate::provable);
  //cout << "\nPred: " << pred.to_string(true) << endl;
  //unordered_set<pair<edge_guard_t, predicate>> preds_apply_src = src_tfg->apply_trans_funs(guard.get_edge_composition(), pred, true);
  //cout << "\nApply trans_func:\n";
  //for (auto const& pp : preds_apply_src) {
  //  cout << "Guard: " << pp.first.edge_guard_to_string() << "\nExpr: " << pp.second.to_string(true) << "\n\n";
  //}

  bool ret;
  query_comment qc(__func__);
  list<counter_example_t> returned_ces;
  ret = corr_graph::prove_using_local_sprel_expr_guesses(&ctx, lhs_set, precond, lsprel_guesses, sprel_map_pair, src_suffixpath, src_pred_avail_exprs, memlabel_map, all_guesses, src, dst, qc, false, *mlasserts, alias_cons, *src_tfg, pcpair::start(), returned_ces);
  cout << (ret ? "EQUAL" : "NOT-EQUAL") << "\n";
  cout << "local sprel expr guesses required:\n" << lsprel_guesses.to_string_for_eq(true) << endl;

  expr_ref evale = nullptr;
  map<expr_id_t, expr_ref> evale_map;
  if (eval_expr.get_value() != "") {
    ifstream ein(eval_expr.get_value());
    if(ein.is_open()) {
      read_expr_and_expr_map(ein, evale, evale_map, &ctx);
    }
    else {
      cout << __func__ << " " << __LINE__ << ": could not open " << expr_file.get_value() << "." << endl;
    }
  }
  for (auto const& ce : returned_ces) {
    DYN_DEBUG(smt_query, cout << ce.counter_example_to_string() << endl);

    map<expr_id_t, expr_ref> src_eval = evaluate_counter_example_on_expr_map(src_map, relevant_memlabels_set, ce);
    expr_print_with_ce_visitor src_visitor(src, src_map, src_eval, cout);

    map<expr_id_t, expr_ref> dst_eval = evaluate_counter_example_on_expr_map(dst_map, relevant_memlabels_set, ce);
    expr_print_with_ce_visitor dst_visitor(dst, dst_map, dst_eval, cout);

    DYN_DEBUG2(smt_query,
        cout << "===\nevaluation src:\n===\n";
        src_visitor.print_result();
        cout << "===\nevaluation dst:\n===\n";
        dst_visitor.print_result()
    );
    if (evale) {
      map<expr_id_t, expr_ref> evale_eval = evaluate_counter_example_on_expr_map(evale_map, relevant_memlabels_set, ce);
      expr_print_with_ce_visitor evale_visitor(evale, evale_map, evale_eval, cout);
      cout << "===\nevaluation " << eval_expr.get_value() << ":\n===\n";
      evale_visitor.print_result();
    }
  }
  //cout << __func__ << " " << __LINE__ << ":\n" << stats::get();
  //cout << ctx.stat() << endl;
  solver_kill();
  call_on_exit_function();

  exit(0);
}
