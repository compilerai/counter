#include "eq/eqcheck.h"
#include "eq/parse_input_eq_file.h"
#include "expr/consts_struct.h"
#include "support/mytimer.h"
#include "support/log.h"
#include "support/cl.h"
#include "support/globals.h"
#include "support/dshared_ptr.h"
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

  init_dyn_debug_from_string(debug.get_value());
  CPP_DBG_EXEC(DYN_DEBUG, print_debug_class_levels());
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

  list<guarded_pred_t> lhs_set_a;
  precond_t precond_a;
  sprel_map_pair_t sprel_map_pair_a;
  tfg_suffixpath_t src_suffixpath_a;
  avail_exprs_t src_avail_exprs_a;
  aliasing_constraints_t alias_cons_a;
  graph_memlabel_map_t memlabel_map_a;
  expr_ref src_a, dst_a;
  map<expr_id_t, expr_ref> src_map_a, dst_map_a;
  map<expr_id_t, pair<expr_ref, expr_ref>> concrete_address_submap_a;
  dshared_ptr<tfg_llvm_t> src_tfg_a;
  graph_edge_composition_ref<pc, tfg_edge> eg_a;
  relevant_memlabels_t relevant_memlabels({});

  read_lhs_set_guard_etc_and_src_dst_from_file(in_a, &ctx, lhs_set_a, precond_a, eg_a, sprel_map_pair_a, src_suffixpath_a, src_avail_exprs_a, alias_cons_a, memlabel_map_a, src_a, dst_a, src_map_a, dst_map_a, concrete_address_submap_a, src_tfg_a, relevant_memlabels);

  list<guarded_pred_t> lhs_set_b;
  precond_t precond_b;
  sprel_map_pair_t sprel_map_pair_b;
  tfg_suffixpath_t src_suffixpath_b;
  avail_exprs_t src_avail_exprs_b;
  aliasing_constraints_t alias_cons_b;
  graph_memlabel_map_t memlabel_map_b;
  expr_ref src_b, dst_b;
  map<expr_id_t, expr_ref> src_map_b, dst_map_b;
  map<expr_id_t, pair<expr_ref, expr_ref>> concrete_address_submap_b;
  dshared_ptr<tfg_llvm_t> src_tfg_b;
  graph_edge_composition_ref<pc, tfg_edge> eg_b;

  read_lhs_set_guard_etc_and_src_dst_from_file(in_b, &ctx, lhs_set_b, precond_b, eg_b, sprel_map_pair_b, src_suffixpath_b, src_avail_exprs_b, alias_cons_b, memlabel_map_b, src_b, dst_b, src_map_b, dst_map_b, concrete_address_submap_b, src_tfg_b, relevant_memlabels);

  list<guarded_pred_t> diff_a, diff_b;
  diff_a = guarded_pred_set_difference(lhs_set_a, lhs_set_b);
  diff_b = guarded_pred_set_difference(lhs_set_b, lhs_set_a);
  cout << "In first only: " <<  diff_a.size() << " preds\n";
  int i = 0;
  for (auto const& p : diff_a) {
    stringstream ss;
    ss << "first-only.pred." << i++;
    cout << ss.str() << endl;
    guarded_pred_to_stream(cout, p, ss.str());
    cout << endl;
  }
  cout << "In second only: " << diff_b.size() << " preds\n";
  i = 0;
  for (auto const& p : diff_b) {
    stringstream ss;
    ss << "second-only.pred." << i++;
    cout << ss.str() << endl;
    guarded_pred_to_stream(cout, p, ss.str());
    cout << endl;
  }

  solver_kill();
  call_on_exit_function();

  exit(0);
}
