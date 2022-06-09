#include "eq/eqcheck.h"
#include "eq/parse_input_eq_file.h"
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
  cl::arg<bool> disable_unaliased_memslot_conversion(cl::explicit_flag, "disable_unaliased_memslot_conversion", false, "Disable unaliased memslot to free var conversion");
  cl::cl cmd("Split out LHS set into two separate files");
  cmd.add_arg(&expr_file);
  cmd.add_arg(&smt_query_timeout);
  cmd.add_arg(&sage_query_timeout);
  cmd.add_arg(&eval_expr);
  cmd.add_arg(&debug);
  cmd.add_arg(&disable_unaliased_memslot_conversion);
  cmd.parse(argc, argv);

  init_dyn_debug_from_string(debug.get_value());
  CPP_DBG_EXEC(DYN_DEBUG, print_debug_class_levels());
  context::config cfg(smt_query_timeout.get_value(), sage_query_timeout.get_value());
  if (disable_unaliased_memslot_conversion.get_value())
    cfg.disable_unaliased_memslot_conversion = true;

  context ctx(cfg);
  ctx.parse_consts_db(CONSTS_DB_FILENAME);
  consts_struct_t &cs = ctx.get_consts_struct();
  solver_init();

  g_query_dir_init();

  ifstream in(expr_file.get_value());
  if(!in.is_open()) {
    cout << __func__ << " " << __LINE__ << ": could not open " << expr_file.get_value() << "." << endl;
    return 1;
  }

  list<guarded_pred_t> lhs_set;
  precond_t precond;
  //local_sprel_expr_guesses_t lsprel_guesses;
  sprel_map_pair_t sprel_map_pair;
  tfg_suffixpath_t src_suffixpath;
  avail_exprs_t src_avail_exprs;
  aliasing_constraints_t alias_cons;
  graph_memlabel_map_t memlabel_map;
  expr_ref src, dst;
  map<expr_id_t, expr_ref> src_map, dst_map;
  map<expr_id_t, pair<expr_ref, expr_ref>> concrete_address_submap;
  dshared_ptr<tfg_llvm_t> src_tfg;
  graph_edge_composition_ref<pc, tfg_edge> eg;
  relevant_memlabels_t relevant_memlabels({});

  read_lhs_set_guard_etc_and_src_dst_from_file(in, &ctx, lhs_set, precond, eg/*, lsprel_guesses*/, sprel_map_pair, src_suffixpath, src_avail_exprs, alias_cons, memlabel_map, src, dst, src_map, dst_map, concrete_address_submap, src_tfg, relevant_memlabels);
  if (lhs_set.size() < 2) {
    return 0;
  }

  list<guarded_pred_t> left_half_pred_set, right_half_pred_set;
  auto mid = next(lhs_set.begin(), lhs_set.size()/2);
  copy(lhs_set.begin(), mid, inserter(left_half_pred_set, left_half_pred_set.end()));
  copy(mid, lhs_set.end(), inserter(right_half_pred_set, right_half_pred_set.end()));

  ostringstream os1;
  os1 << expr_file.get_value() << "_l";
  ofstream out1(os1.str());
  if (!out1.is_open()) {
    cout << __func__ << " " << __LINE__ << ": could not open file for writing: " << os1.str() << endl;
    return 2;
  }
  ostringstream os2;
  os2 << expr_file.get_value() << "_r";
  ofstream out2(os2.str());
  if (!out2.is_open()) {
    cout << __func__ << " " << __LINE__ << ": could not open file for writing: " << os2.str() << endl;
    return 3;
  }

  output_lhs_set_guard_etc_and_src_dst_to_file(out1, left_half_pred_set, precond, eg, /*lsprel_guesses, */sprel_map_pair, src_suffixpath, src_avail_exprs, memlabel_map, src, dst, concrete_address_submap, relevant_memlabels, alias_cons, *src_tfg);
  output_lhs_set_guard_etc_and_src_dst_to_file(out2, right_half_pred_set, precond, eg, /*lsprel_guesses, */sprel_map_pair, src_suffixpath, src_avail_exprs, memlabel_map, src, dst, concrete_address_submap, relevant_memlabels, alias_cons, *src_tfg);

  cout << os1.str() << endl;
  cout << os2.str() << endl;

  call_on_exit_function();

  return 0;
}
