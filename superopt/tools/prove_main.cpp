#include <fstream>
#include <iostream>
#include <string>

#include "support/mytimer.h"
#include "support/log.h"
#include "support/cl.h"
#include "support/globals.h"
#include "support/timers.h"
#include "support/dyn_debug.h"

#include "expr/expr.h"
#include "expr/expr_utils.h"
#include "expr/consts_struct.h"
#include "expr/z3_solver.h"

#include "graph/prove_spawn_and_join.h"

#include "eq/eqcheck.h"
#include "eq/parse_input_eq_file.h"
#include "eq/corr_graph.h"

using namespace std;

int main(int argc, char **argv)
{
  // command line args processing
  cl::arg<string> expr_file(cl::implicit_prefix, "", "path to input file");
  cl::arg<unsigned> smt_query_timeout(cl::explicit_prefix, "smt-query-timeout", 600, "Timeout per query (s)");
  cl::arg<unsigned> sage_query_timeout(cl::explicit_prefix, "sage-query-timeout", 600, "Timeout per query (s)");
  cl::arg<string> debug(cl::explicit_prefix, "dyn-debug", "", "Enable dynamic debugging for debug-class(es).  Expects comma-separated list of debug-classes with optional level e.g. --debug=correlate=2,smt_query=1");
  cl::arg<string> eval_expr(cl::explicit_prefix, "eval-expr", "", "Evaluate the returned counter example (if any) on expr in this file");
  cl::arg<bool> disable_unaliased_memslot_conversion(cl::explicit_flag, "disable_unaliased_memslot_conversion", false, "Disable unaliased memslot to free var conversion");
  cl::arg<bool> enable_CE_fuzzing(cl::explicit_flag, "enable_CE_fuzzing", false, "Enable the Random array constant fuzzing the Es retyrned by solverl");
  cl::arg<bool> enable_query_decomposition(cl::explicit_flag, "enable-query-decomposition", false, "Enable Query decomposition");
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
  cmd.add_arg(&disable_unaliased_memslot_conversion);
  cmd.add_arg(&enable_CE_fuzzing);
  cmd.add_arg(&enable_query_decomposition);
  cmd.parse(argc, argv);

  init_dyn_debug_from_string(debug.get_value());

  DYN_DBG_SET(prove_debug, max(get_debug_class_level("prove_path"), 1));

  CPP_DBG_EXEC(DYN_DEBUG, print_debug_class_levels());

  context::config cfg(smt_query_timeout.get_value(), sage_query_timeout.get_value());
  if (disable_unaliased_memslot_conversion.get_value())
    cfg.disable_unaliased_memslot_conversion = true;
  if (enable_CE_fuzzing.get_value())
    cfg.enable_CE_fuzzing = true;
  if (enable_query_decomposition.get_value())
    cfg.enable_query_decomposition = true;

  context ctx(cfg);
  ctx.parse_consts_db(CONSTS_DB_FILENAME);
  consts_struct_t &cs = ctx.get_consts_struct();
  solver_init();
  g_query_dir_init();

  DYN_DBG_SET(ce_add, 4);
  //DBG_SET(UNALIASED_MEMSLOT, 2);

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
  map<expr_id_t, pair<expr_ref, expr_ref>> concrete_address_submap;
  //dshared_ptr<memlabel_assertions_t> mlasserts;
  dshared_ptr<tfg_llvm_t> src_tfg;
  graph_edge_composition_ref<pc, tfg_edge> ec;

  relevant_memlabels_t relevant_memlabels({});

  read_lhs_set_guard_etc_and_src_dst_from_file(in, &ctx, lhs_set, precond, ec, sprel_map_pair, src_suffixpath, src_avail_exprs, alias_cons, memlabel_map, src, dst, src_map, dst_map, concrete_address_submap, src_tfg, relevant_memlabels);

  set<memlabel_ref> const& relevant_memlabels_set = relevant_memlabels.relevant_memlabels_get_ml_set();

  DYN_DBG_SET(smt_query_dump, 2);
  DYN_DBG_SET(smt_file, 1);

  src_tfg->populate_auxilliary_structures_dependent_on_locs();
  src_tfg->populate_loc_and_var_definedness();
  //src_tfg->populate_simplified_assets();

  query_comment qc(__func__);
  proof_status_t ret;
  list<counter_example_t> returned_ces;
  tie(ret, returned_ces) = prove_spawn_and_join(&ctx, lhs_set, precond, ec, sprel_map_pair, src_suffixpath, src_avail_exprs, memlabel_map, src, dst, qc/*, false*/, concrete_address_submap, relevant_memlabels, alias_cons, *src_tfg/*, pcpair::start()*//*, returned_ces*/);
  cout << proof_status_to_string(ret) << "\n";

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
  for (auto ce : returned_ces) {
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
    if (evale) {
      map<expr_id_t, expr_ref> evale_eval = evaluate_counter_example_on_expr_map(evale_map, relevant_memlabels_set, ce);
      expr_print_with_ce_visitor evale_visitor(evale, evale_map, evale_eval, cout);
      cout << "===\nevaluation " << eval_expr.get_value() << ":\n===\n";
      evale_visitor.print_result();
    }
    if (dst->get_operation_kind() == expr::OP_MEMMASKS_ARE_EQUAL) {
      // show evaluated masked memories
      expr_ref mem1 = dst->get_args().at(OP_MEMMASKS_ARE_EQUAL_ARGNUM_MEM1);
      expr_ref mem2 = dst->get_args().at(OP_MEMMASKS_ARE_EQUAL_ARGNUM_MEM2);
      expr_ref mem_alloc1 = dst->get_args().at(OP_MEMMASKS_ARE_EQUAL_ARGNUM_MEM_ALLOC1);
      expr_ref mem_alloc2 = dst->get_args().at(OP_MEMMASKS_ARE_EQUAL_ARGNUM_MEM_ALLOC2);
      memlabel_t ml = dst->get_args().at(OP_MEMMASKS_ARE_EQUAL_ARGNUM_MEMLABEL)->get_memlabel_value();

      expr_ref masked_mem1 = ctx.mk_memmask(mem1, mem_alloc1, ml);
      expr_ref masked_mem2 = ctx.mk_memmask(mem2, mem_alloc2, ml);

      expr_ref eval_masked_mem1, eval_masked_mem2;
      {
        counter_example_t rand_vals(&ctx, ctx.get_next_ce_name(RAND_VALS_CE_NAME_PREFIX));
        bool success = ce.evaluate_expr_assigning_random_consts_as_needed(masked_mem1, eval_masked_mem1, rand_vals, relevant_memlabels_set);
        ASSERT(success);
      }
      {
        counter_example_t rand_vals(&ctx, ctx.get_next_ce_name(RAND_VALS_CE_NAME_PREFIX));
        bool success = ce.evaluate_expr_assigning_random_consts_as_needed(masked_mem2, eval_masked_mem2, rand_vals, relevant_memlabels_set);
        ASSERT(success);
      }

      cout << "=== memmasks_are_equal -- evaluation of mem1 ===\n";
      cout << ctx.expr_to_string_table(eval_masked_mem1) << endl;
      cout << "=== memmasks_are_equal -- evaluation of mem2 ===\n";
      cout << ctx.expr_to_string_table(eval_masked_mem2) << endl;
    }
  }
  //cout << __func__ << " " << __LINE__ << ":\n" << stats::get();
  //cout << ctx.stat() << endl;
  solver_kill();
  call_on_exit_function();

  exit(0);
}
