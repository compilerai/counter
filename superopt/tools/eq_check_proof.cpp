#include "support/log.h"
#include "eq/eqcheck.h"
#include "tfg/parse_input_eq_file.h"
#include "expr/consts_struct.h"
#include "support/mytimer.h"
#include "support/cl.h"
#include "support/globals.h"

#include "support/timers.h"

#include "expr/z3_solver.h"

#include <iostream>
#include <string>

#define DEBUG_TYPE "eq-check-proof"

using namespace std;

bool test_graph();
bool test_sub_expr();

int
main(int argc, char **argv)
{
  // command line args processing
  cl::arg<string> function_name(cl::implicit_prefix, "", "function name to be compared for equivalence");
  cl::arg<string> etfg_file(cl::implicit_prefix, "", "path to .etfg file");
  cl::arg<string> tfg_file(cl::implicit_prefix, "", "path to .tfg file");
  cl::arg<string> proof_file(cl::implicit_prefix, "", "path to .proof file");
  cl::arg<unsigned> smt_query_timeout(cl::explicit_prefix, "smt-query-timeout", 3600, "Timeout per query (s)");
  cl::arg<unsigned> sage_query_timeout(cl::explicit_prefix, "sage-query-timeout", 3600, "Timeout per query (s)");
  cl::arg<unsigned> total_timeout(cl::explicit_prefix, "total-timeout", 3600, "Total timeout (s)");
  cl::arg<unsigned long> max_address_space_size(cl::explicit_prefix, "max-address-space-size", (unsigned long)-1, "Max address space size");
  cl::cl cmd("Equivalence proof checker: checks equivalence of given .eq file with given .proof file");
  cmd.add_arg(&function_name);
  cmd.add_arg(&etfg_file);
  cmd.add_arg(&tfg_file);
  cmd.add_arg(&proof_file);
  cmd.add_arg(&smt_query_timeout);
  cmd.add_arg(&sage_query_timeout);
  cmd.add_arg(&total_timeout);
  cmd.add_arg(&max_address_space_size);
  cmd.parse(argc, argv);

  quota qt(1, smt_query_timeout.get_value(), sage_query_timeout.get_value(), total_timeout.get_value(), max_address_space_size.get_value(), 0);
  g_query_dir_init();

  context::config cfg(qt.get_smt_query_timeout(), qt.get_sage_query_timeout());
  context ctx(cfg);
  vector<dst_insn_t> dst_iseq_vec;
  vector<dst_ulong> dst_insn_pcs_vec;
  //consts_struct_t cs;
  //cs.parse_consts_db(&ctx);
  ctx.parse_consts_db(CONSTS_DB_FILENAME);
  consts_struct_t &cs = ctx.get_consts_struct();
  tfg *t_llvm = NULL, *t_dst = NULL;
  rodata_map_t rodata_map;
  //cout << "eq_file: " << eq_file.get_value() << endl;
  ifstream ifs(tfg_file.get_value());
  ASSERT(ifs.good());
  bool check = parse_input_eq_file(function_name.get_value(), etfg_file.get_value(), ifs, &t_dst, &t_llvm, rodata_map, dst_iseq_vec, dst_insn_pcs_vec, &ctx, false);
  if (!check) {
    cout << "Invalid eq file" << endl;
    exit(1); //return 1 if does not need further exploration
  }
  t_llvm->replace_alloca_with_nop();
  t_dst->replace_alloca_with_nop();
  //vector<memlabel_ref> relevant_memlabels;
  //tfgs_get_relevant_memlabels(relevant_memlabels, *t_llvm, *t_dst);
  //cs.solver_set_relevant_memlabels(relevant_memlabels);


  for (auto e : t_llvm->get_argument_regs()) {
    cout << __func__ << " " << __LINE__ << ": src arg: " << expr_string(e.second) << endl;
  }
  for (auto e : t_dst->get_argument_regs()) {
    cout << __func__ << " " << __LINE__ << ": dst arg: " << expr_string(e.second) << endl;
  }

  //cs.set_argument_constants(t_llvm->get_argument_regs());

  t_llvm->collapse_tfg(true);
  t_dst->collapse_tfg(true/*false*/);

  t_llvm->populate_transitive_closure();
  t_llvm->populate_reachable_and_unreachable_incoming_edges_map();
  t_dst->populate_transitive_closure();
  t_dst->populate_reachable_and_unreachable_incoming_edges_map();

  //populate edge2loc_map for t_llvm/t_dst; helps with guessing
  t_llvm->populate_auxilliary_structures_dependent_on_locs();
  //t_llvm_copy->populate_auxilliary_structures_dependent_on_locs();
  t_dst->populate_auxilliary_structures_dependent_on_locs();

  //t_llvm->resize_farg_exprs_for_function_calling_conventions();
 
  fixed_reg_mapping_t dst_fixed_reg_mapping = fixed_reg_mapping_t::default_fixed_reg_mapping_for_function_granular_eq();
  shared_ptr<eqspace::eqcheck> e = make_shared<eqspace::eqcheck>(*t_llvm, *t_dst, dst_fixed_reg_mapping, rodata_map, &ctx, false, qt/*, true*/);
  if (e->check_proof(relevant_memlabels, proof_file.get_value())) {
    LOG("eq-proof-valid:\n" << "\n-------- SMILE! :) --------\n" << endl);
  } else {
    LOG("eq-proof-invalid:\n" << endl);
  }
  
  solver_kill();
  call_on_exit_function();
  exit(0);
}
