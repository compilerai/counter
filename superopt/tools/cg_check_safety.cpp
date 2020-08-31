#include "support/log.h"
#include "eq/eqcheck.h"
#include "tfg/parse_input_eq_file.h"
#include "expr/consts_struct.h"
#include "eq/corr_graph.h"
#include "support/mytimer.h"
#include "support/cl.h"
#include "support/globals.h"

#include "support/timers.h"

#include "expr/z3_solver.h"
#include "eq/cg_with_safety.h"

#include <iostream>
#include <string>

using namespace std;

int
main(int argc, char **argv)
{
  // command line args processing
  cl::arg<string> etfg_file(cl::implicit_prefix, "", "path to .etfg file");
  cl::arg<string> tfg_file(cl::implicit_prefix, "", "path to .tfg file");
  cl::arg<string> proof_file(cl::implicit_prefix, "", "path to .proof file");
  cl::arg<string> function_name(cl::implicit_prefix, "", "function name to be checked for safety");
  cl::cl cmd("CG safety checker: checks that the safety conditions are met in the CG available in the proof file");
  cmd.add_arg(&etfg_file);
  cmd.add_arg(&tfg_file);
  cmd.add_arg(&proof_file);
  cmd.add_arg(&function_name);
  cmd.parse(argc, argv);

  quota qt(1, 3600, 3600, 36000, (unsigned long)-1, 0);
  g_query_dir_init();

  context::config cfg(3600, 3600);
  g_ctx_init();
  solver_init();
  context *ctx = g_ctx;
  //context ctx(cfg);
  //consts_struct_t cs;
  //cs.parse_consts_db(&ctx);
  ctx->set_config(cfg);
  ctx->parse_consts_db(CONSTS_DB_FILENAME);
  consts_struct_t &cs = ctx->get_consts_struct();
  tfg *t_llvm = NULL, *t_dst = NULL;
  rodata_map_t rodata_map;
  //cout << "eq_file: " << eq_file.get_value() << endl;
  ifstream ifs(tfg_file.get_value());
  ASSERT(ifs.good());
  string cur_function_name;
  while (getNextFunctionInTfgFile(ifs, cur_function_name)) {
    if (cur_function_name != function_name.get_value()) {
      continue;
    }
    vector<dst_insn_t> dst_iseq_vec;
    vector<dst_ulong> dst_insn_pcs_vec;
    bool check = parse_input_eq_file(function_name.get_value(), etfg_file.get_value(), ifs, &t_dst, &t_llvm, rodata_map, dst_iseq_vec, dst_insn_pcs_vec, ctx, false);
    if (!check) {
      cout << "Invalid eq file" << endl;
      exit(1); //return 1 if does not need further exploration
    }
    fixed_reg_mapping_t dst_fixed_reg_mapping = fixed_reg_mapping_t::default_fixed_reg_mapping_for_function_granular_eq();
    shared_ptr<eqspace::eqcheck> e = make_shared<eqspace::eqcheck>(function_name.get_value(), *t_llvm, *t_dst, dst_fixed_reg_mapping, rodata_map, dst_iseq_vec, dst_insn_pcs_vec, ctx, false, qt);

    corr_graph const& cg = e->corr_graph_read_from_file(proof_file.get_value());

    cg_with_safety cg_safety(cg);
    if (!cg_safety.check_safety()) {
      cout << __func__ << " " << __LINE__ << ": Safety check failed on CG!\n";
    }
  }
  solver_kill();
  call_on_exit_function();
  exit(0);
}
