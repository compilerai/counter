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
  cl::arg<string> file_a(cl::implicit_prefix, "", "path to input file");
  cl::arg<string> file_b(cl::implicit_prefix, "", "path to input file");
  cl::arg<string> debug(cl::explicit_prefix, "debug", "", "Enable dynamic debugging for debug-class(es).  Expects comma-separated list of debug-classes with optional level e.g. --debug=correlate=2,smt_query=1");

  cl::cl cmd("Print difference between predicate sets of two .prove files");
  cmd.add_arg(&file_a);
  cmd.add_arg(&file_b);
  cmd.add_arg(&debug);
  cmd.parse(argc, argv);

  eqspace::init_dyn_debug_from_string(debug.get_value());
  CPP_DBG_EXEC(DYN_DEBUG, eqspace::print_debug_class_levels());
  context::config cfg(100, 100);
  context ctx(cfg);
  ctx.parse_consts_db(CONSTS_DB_FILENAME);
  consts_struct_t &cs = ctx.get_consts_struct();

  g_query_dir_init();

  ifstream in_a(file_a.get_value());
  if(!in_a.is_open()) {
    cout << __func__ << " " << __LINE__ << ": could not open " << file_a.get_value() << "." << endl;
    exit(1);
  }

  ifstream in_b(file_b.get_value());
  if(!in_b.is_open()) {
    cout << __func__ << " " << __LINE__ << ": could not open " << file_b.get_value() << "." << endl;
    exit(1);
  }

  predicate_set_t lhs_set_a;
  precond_t precond_a;
  local_sprel_expr_guesses_t lsprel_guesses_a;
  sprel_map_pair_t sprel_map_pair_a;
  tfg_suffixpath_t src_suffixpath_a;
  pred_avail_exprs_t src_pred_avail_exprs_a;
  aliasing_constraints_t alias_cons_a;
  graph_memlabel_map_t memlabel_map_a;
  set<local_sprel_expr_guesses_t> all_guesses_a;
  expr_ref src_a, dst_a;
  map<expr_id_t, expr_ref> src_map_a, dst_map_a;
  shared_ptr<memlabel_assertions_t> mlasserts_a;
  shared_ptr<tfg> src_tfg_a;

  read_lhs_set_guard_lsprels_and_src_dst_from_file(in_a, &ctx, lhs_set_a, precond_a, lsprel_guesses_a, sprel_map_pair_a, src_suffixpath_a, src_pred_avail_exprs_a, alias_cons_a, memlabel_map_a, all_guesses_a, src_a, dst_a, src_map_a, dst_map_a, mlasserts_a, src_tfg_a);

  predicate_set_t lhs_set_b;
  precond_t precond_b;
  local_sprel_expr_guesses_t lsprel_guesses_b;
  sprel_map_pair_t sprel_map_pair_b;
  tfg_suffixpath_t src_suffixpath_b;
  pred_avail_exprs_t src_pred_avail_exprs_b;
  aliasing_constraints_t alias_cons_b;
  graph_memlabel_map_t memlabel_map_b;
  set<local_sprel_expr_guesses_t> all_guesses_b;
  expr_ref src_b, dst_b;
  map<expr_id_t, expr_ref> src_map_b, dst_map_b;
  shared_ptr<memlabel_assertions_t> mlasserts_b;
  shared_ptr<tfg> src_tfg_b;

  read_lhs_set_guard_lsprels_and_src_dst_from_file(in_b, &ctx, lhs_set_b, precond_b, lsprel_guesses_b, sprel_map_pair_b, src_suffixpath_b, src_pred_avail_exprs_b, alias_cons_b, memlabel_map_b, all_guesses_b, src_b, dst_b, src_map_b, dst_map_b, mlasserts_b, src_tfg_b);

  predicate_set_t diff_a, diff_b;
  predicate_set_difference(lhs_set_a, lhs_set_b, diff_a);
  predicate_set_difference(lhs_set_b, lhs_set_a, diff_b);
  cout << "In first only: " <<  diff_a.size() << " preds\n";
  int i = 0;
  for (auto const& p : diff_a) {
    cout << "first-only.pred." << i++ << endl;
    cout << p->to_string_for_eq() << endl;
  }
  cout << "In second only: " << diff_b.size() << " preds\n";
  i = 0;
  for (auto const& p : diff_b) {
    cout << "second-only.pred." << i++ << endl;
    cout << p->to_string_for_eq() << endl;
  }

  solver_kill();
  call_on_exit_function();

  exit(0);
}
