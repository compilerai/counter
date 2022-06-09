#include "eq/eqcheck.h"
#include "eq/parse_input_eq_file.h"
#include "expr/consts_struct.h"
#include "support/mytimer.h"
#include "support/log.h"
#include "support/cl.h"
#include "support/globals.h"
#include "expr/expr.h"
#include "expr/expr_utils.h"

#include "support/timers.h"
#include "expr/z3_solver.h"

#include "support/dyn_debug.h"
#include "eq/corr_graph.h"
#include <fstream>
#include <iostream>
#include <string>

using namespace std;

int main(int argc, char **argv)
{
  // command line args processing
  cl::arg<string> expr_file(cl::implicit_prefix, "", "path to input file");
  cl::arg<string> ce_file(cl::implicit_prefix, "", "path to input CE file");
  cl::arg<unsigned> smt_query_timeout(cl::explicit_prefix, "smt-query-timeout", 600, "Timeout per query (s)");
  cl::arg<string> debug(cl::explicit_prefix, "dyn-debug", "", "Enable dynamic debugging for debug-class(es).  Expects comma-separated list of debug-classes with optional level e.g. --debug=correlate=2,smt_query=1");
  cl::arg<bool> disable_unaliased_memslot_conversion(cl::explicit_flag, "disable_unaliased_memslot_conversion", false, "Disable unaliased memslot to free var conversion");
  cl::arg<bool> enable_CE_fuzzing(cl::explicit_flag, "enable_CE_fuzzing", false, "Enable the Random array constant fuzzing the Es retyrned by solverl");
  cl::cl cmd("Eval prove file's lhs and rhs over passed CE.");

  cmd.add_arg(&expr_file);
  cmd.add_arg(&ce_file);
  cmd.add_arg(&smt_query_timeout);
  cmd.add_arg(&debug);
  cmd.add_arg(&disable_unaliased_memslot_conversion);
  cmd.add_arg(&enable_CE_fuzzing);
  cmd.parse(argc, argv);

  init_dyn_debug_from_string(debug.get_value());
  CPP_DBG_EXEC(DYN_DEBUG, print_debug_class_levels());

  context::config cfg(smt_query_timeout.get_value());
  if (disable_unaliased_memslot_conversion.get_value())
    cfg.disable_unaliased_memslot_conversion = true;
  if (enable_CE_fuzzing.get_value())
    cfg.enable_CE_fuzzing = true;

  context ctx(cfg);
  ctx.parse_consts_db(CONSTS_DB_FILENAME);
  consts_struct_t &cs = ctx.get_consts_struct();
  solver_init();

  g_query_dir_init();

  DYN_DBG_SET(ce_add, 4);

  ifstream in(expr_file.get_value());
  if(!in.is_open()) {
    cout << __func__ << " " << __LINE__ << ": could not open " << expr_file.get_value() << "." << endl;
    exit(1);
  }

  ifstream ce_in(ce_file.get_value());
  if(!ce_in.is_open()) {
    cout << __func__ << " " << __LINE__ << ": could not open " << ce_file.get_value() << "." << endl;
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
  graph_edge_composition_ref<pc, tfg_edge> ec;
  map<expr_id_t, pair<expr_ref, expr_ref>> concrete_address_submap;
  relevant_memlabels_t relevant_memlabels({});

  read_lhs_set_guard_etc_and_src_dst_from_file(in, &ctx, lhs_set, precond, ec, sprel_map_pair, src_suffixpath, src_avail_exprs, alias_cons, memlabel_map, src, dst, src_map, dst_map, concrete_address_submap, src_tfg, relevant_memlabels);

  set<memlabel_ref> const& relevant_memlabels_set = relevant_memlabels.relevant_memlabels_get_ml_set();

  DYN_DBG_SET(smt_query_dump, 2);
  DYN_DBG_SET(smt_file, 1);

  //src_tfg->populate_auxilliary_structures_dependent_on_locs();

  counter_example_t input_ce(&ctx, "ce_eval_prove");
  input_ce.counter_example_from_stream(ce_in, &ctx);

  map<expr_id_t, expr_ref> evale_map;
  for (auto ce : { input_ce }) {
    DYN_DEBUG(smt_query_dump, cout << ce.counter_example_to_string() << endl);

    map<expr_id_t, expr_ref> src_eval = evaluate_counter_example_on_expr_map(src_map, relevant_memlabels_set, ce);
    expr_print_with_ce_visitor src_visitor(src, src_map, src_eval, cout);

    map<expr_id_t, expr_ref> dst_eval = evaluate_counter_example_on_expr_map(dst_map, relevant_memlabels_set, ce);
    expr_print_with_ce_visitor dst_visitor(dst, dst_map, dst_eval, cout);

    DYN_DEBUG2(smt_query_dump,
        cout << "===\nevaluation src:\n===\n";
        src_visitor.print_result();
        cout << "===\nevaluation dst:\n===\n";
        dst_visitor.print_result()
    );
  }
  solver_kill();
  call_on_exit_function();

  exit(0);
}
