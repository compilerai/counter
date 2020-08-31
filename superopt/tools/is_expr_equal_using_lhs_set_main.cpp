#include "eq/eqcheck.h"
#include "tfg/parse_input_eq_file.h"
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

#include <fstream>

#include <iostream>
#include <string>

#define DEBUG_TYPE "main"

using namespace std;

int main(int argc, char **argv)
{
  // command line args processing
  cl::arg<string> expr_file(cl::implicit_prefix, "", "path to .expr file");
  cl::arg<unsigned> smt_query_timeout(cl::explicit_prefix, "smt-query-timeout", 600, "Timeout per query (s)");
  cl::arg<unsigned> sage_query_timeout(cl::explicit_prefix, "sage-query-timeout", 600, "Timeout per query (s)");
  cl::arg<bool> syntactic(cl::explicit_flag, "syntactic", false, "Use syntactic check only");
  cl::arg<bool> simplify(cl::explicit_flag, "simplify", false, "Only simplify the input file and output it");
  cl::arg<bool> ignore_all_undef_behaviour_lhs_preds(cl::explicit_flag, "ignore-all-undef-behaviour-lhs-preds", false, "Remove all undef behaviour lhs preds and output the revised query");
  cl::arg<bool> ignore_all_sprel_assume_lhs_preds(cl::explicit_flag, "ignore-all-sprel-assume-lhs-preds", false, "Remove all sprel assume lhs preds and output the revised query");
  cl::cl cmd("Checks if two expressions are equal under preconditions (lhs set)");
  cmd.add_arg(&expr_file);
  cmd.add_arg(&smt_query_timeout);
  cmd.add_arg(&sage_query_timeout);
  cmd.add_arg(&syntactic);
  cmd.add_arg(&simplify);
  cmd.add_arg(&ignore_all_undef_behaviour_lhs_preds);
  cmd.add_arg(&ignore_all_sprel_assume_lhs_preds);
  cmd.parse(argc, argv);

  context::config cfg(smt_query_timeout.get_value(), sage_query_timeout.get_value());
  context ctx(cfg);
  //consts_struct_t cs;
  //cs.parse_consts_db(&ctx);
  ctx.parse_consts_db(CONSTS_DB_FILENAME);
  consts_struct_t &cs = ctx.get_consts_struct();

  ifstream in(expr_file.get_value());
  if(!in.is_open()) {
    cout << __func__ << " " << __LINE__ << ": could not open " << expr_file.get_value() << "." << endl;
    exit(1);
  }

  //set<pair<expr_ref, pair<expr_ref, expr_ref>>> lhs_set;
  predicate_set_t lhs_set;
  expr_ref src, dst;
  string line;
  bool end;
  tfg *src_tfg;

  g_query_dir_init();

  getline(in, line);
  ASSERT(is_line(line, "=TFG"));
  //line = read_tfg(in, &src_tfg, "llvm", &ctx, false).second;
  src_tfg = new tfg(in, "llvm", &ctx);
  //line = src_tfg->graph_from_stream(in);

  do {
    getline(in, line);
  } while (line == "");
  set<cs_addr_ref_id_t> relevant_addr_refs;
  line = cs.read_relevant_addr_refs(in, line, relevant_addr_refs);

  while (is_line(line, "=lhs-pred")) {
    predicate_ref pred;
    line = predicate::read_pred_and_insert_into_set(in/*, src_tfg*/, &ctx, pred);
    lhs_set.insert(*pred);
  }
  /*while (is_line(line, "=lhs-precond-guard")) {
    expr_ref guard, src, dst;
    line = read_expr(in, guard, &ctx);
    ASSERT(is_line(line, "=lhs-precond-src"));
    line = read_expr(in, src, &ctx);
    ASSERT(is_line(line, "=lhs-precond-dst"));
    line = read_expr(in, dst, &ctx);
    lhs_set.insert(make_pair(guard, make_pair(src, dst)));
  }*/
  local_sprel_expr_guesses_t lsprel_guesses;
  tfg_suffixpath_t src_suffixpath;
  pred_avail_exprs_t src_pred_avail_exprs;
  edge_guard_t guard;
  aliasing_constraints_t rhs_aliasing_cons;
  ASSERT(is_line(line, "=edgeguard"));
  NOT_IMPLEMENTED();
  //line = edge_guard_t::read_edge_guard(in, guard);
  ASSERT(is_line(line, "=lsprels"));
  line = read_local_sprel_expr_assumptions(in, &ctx, lsprel_guesses);
  sprel_map_pair_t sprel_map_pair;
  ASSERT(is_line(line, "=sprel_map_pair"));
  line = read_sprel_map_pair(in, &ctx, sprel_map_pair);
  ASSERT(is_line(line, "=src_suffixpath"));
  line = read_src_suffixpath(in, src_tfg->get_start_state(), &ctx, src_suffixpath);
  ASSERT(src_suffixpath != nullptr);
  ASSERT(is_line(line, "=src_pred_avail_exprs"));
  line = read_src_pred_avail_exprs(in, src_tfg->get_start_state(), &ctx, src_pred_avail_exprs);
  ASSERT(is_line(line, "=aliasing_constraints"));
  line = read_aliasing_constraints(in, src_tfg, &ctx, rhs_aliasing_cons);
  while (line == "") {
    getline(in, line);
  }
  while (line == "") {
    getline(in, line);
  }
  ASSERT(is_line(line, "=src"));
  line = read_expr(in, src, &ctx);
  ASSERT(is_line(line, "=dst"));
  line = read_expr(in, dst, &ctx);
  precond_t precond(guard, lsprel_guesses);

  vector<memlabel_ref> relevant_memlabels;
  //for (auto lhs : lhs_set) {
  //  lhs.get_relevant_memlabels(relevant_memlabels);
  //  /*expr_get_relevant_memlabels(lhs.first, relevant_memlabels);
  //  expr_get_relevant_memlabels(lhs.second.first, relevant_memlabels);
  //  expr_get_relevant_memlabels(lhs.second.second, relevant_memlabels);*/
  //}

  NOT_IMPLEMENTED();
  //expr_get_relevant_memlabels(src, relevant_memlabels);
  //expr_get_relevant_memlabels(dst, relevant_memlabels);
  //cs.solver_set_relevant_memlabels(relevant_memlabels);

  bool ret;
  if (ignore_all_undef_behaviour_lhs_preds.get_value()) {
    predicate_set_t lhs_set_shortened;
    for (auto p : lhs_set) {
      if (p.get_comment().find(UNDEF_BEHAVIOUR_ASSUME_COMMENT_PREFIX) == 0) {
        continue;
      }
      lhs_set_shortened.insert(p);
    }
    output_lhs_set_precond_and_src_dst_to_file(std::cout, lhs_set_shortened, precond, sprel_map_pair, src_suffixpath, src_pred_avail_exprs, src, dst, *src_tfg);
  } else if (ignore_all_sprel_assume_lhs_preds.get_value()) {
    predicate_set_t lhs_set_shortened;
    for (auto p : lhs_set) {
      if (p.get_comment().find(SPREL_ASSUME_COMMENT_PREFIX) == 0) {
        continue;
      }
      lhs_set_shortened.insert(p);
    }
    output_lhs_set_precond_and_src_dst_to_file(std::cout, lhs_set_shortened, precond, sprel_map_pair, src_suffixpath, src_pred_avail_exprs, src, dst, *src_tfg);
  } else if (simplify.get_value()) {
    expr_ref src_simpl = expr_do_simplify_using_lhs_set_and_precond(src, true, lhs_set, precond, sprel_map_pair);
    expr_ref dst_simpl = expr_do_simplify_using_lhs_set_and_precond(dst, true, lhs_set, precond, sprel_map_pair);

    output_lhs_set_precond_and_src_dst_to_file(std::cout, lhs_set, precond, sprel_map_pair, src_suffixpath, src_pred_avail_exprs, src, dst, *src_tfg);
  } else if (syntactic.get_value()) {
    ret = is_expr_equal_syntactic_using_lhs_set_and_precond(src, dst, lhs_set, precond/*, sprel_map_pair, src_suffixpath, src_pred_avail_exprs*/);
    cout << (ret ? "EQUAL" : "NOT-EQUAL") << "\n";
  } else {
    query_comment qc(__func__);
    counter_example_t counter_example;
    map<mlvarname_t, memlabel_t> memlabel_map; //should read this from the input file
    ret = is_expr_equal_using_lhs_set_and_precond(&ctx, lhs_set, precond, sprel_map_pair, src_suffixpath, src_pred_avail_exprs, memlabel_map, src, dst, qc, false, counter_example, relevant_addr_refs, relevant_memlabels, rhs_aliasing_cons, *src_tfg, cs);
    cout << (ret ? "EQUAL" : "NOT-EQUAL") << "\n";
  }

  solver_kill();
  call_on_exit_function();

  exit(0);
}
