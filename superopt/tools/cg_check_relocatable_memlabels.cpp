#include "support/log.h"
#include "support/mytimer.h"
#include "support/cl.h"
#include "support/timers.h"
#include "support/globals.h"

#include "expr/consts_struct.h"
#include "expr/z3_solver.h"

#include "insn/dst-insn.h"
#include "i386/insn.h"
#include "x64/insn.h"
#include "etfg/etfg_insn.h"

#include "ptfg/llptfg.h"

#include "eq/eqcheck.h"
#include "eq/parse_input_eq_file.h"
#include "eq/corr_graph.h"
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
  cl::cl cmd("CG checker with relocatable memlabels: checks that the inductive predicates and assertions are provable with abstract memlabels for the CG available in the proof file");
  cmd.add_arg(&etfg_file);
  cmd.add_arg(&tfg_file);
  cmd.add_arg(&proof_file);
  cmd.add_arg(&function_name);
  cl::arg<string> debug(cl::explicit_prefix, "dyn-debug", "", "Enable dynamic debugging for debug-class(es).  Expects comma-separated list of debug-classes with optional level e.g. --debug=correlate=2,smt_query=1,dumptfg,decide_hoare_triple,update_invariant_state_over_edge");
  cmd.add_arg(&debug);
  cmd.parse(argc, argv);

  init_dyn_debug_from_string(debug.get_value());
  CPP_DBG_EXEC(DYN_DEBUG, print_debug_class_levels());

  g_query_dir_init();

  quota qt;
  context::config cfg(qt.get_smt_query_timeout(), qt.get_sage_query_timeout());
  g_ctx_init();

  solver_init();
  src_init();
  dst_init();
  context *ctx = g_ctx;
  //context ctx(cfg);
  //consts_struct_t cs;
  //cs.parse_consts_db(&ctx);
  ctx->set_config(cfg);
  ctx->parse_consts_db(CONSTS_DB_FILENAME);
  consts_struct_t &cs = ctx->get_consts_struct();

  //shared_ptr<tfg_llvm_t> t_llvm;
  //shared_ptr<tfg> t_dst;
  //rodata_map_t rodata_map;

  ifstream in_llvm(etfg_file.get_value());
  if (!in_llvm.is_open()) {
    cout << __func__ << " " << __LINE__ << ": Could not open etfg_filename '" << etfg_file.get_value() << "'\n";
  }
  llptfg_t llptfg(in_llvm, ctx);
  in_llvm.close();

  map<string, dst_parsed_tfg_t> dst_function_tfg_map = parse_dst_function_tfg_map(tfg_file.get_value(), llptfg, ctx);


  //ifstream ifs(tfg_file.get_value());
  //ASSERT(ifs.good());
  //string cur_function_name;
  //while (getNextFunctionInTfgFile(ifs, cur_function_name))
  for (auto const& fname_tfg : dst_function_tfg_map) {
    string const& cur_function_name = fname_tfg.first;
    //cout << "cur_function_name = " << cur_function_name << endl;
    if (cur_function_name != function_name.get_value()) {
      //skip_till_next_function(ifs);
      continue;
    }
    //vector<dst_insn_t> dst_iseq_vec;
    //vector<dst_ulong> dst_insn_pcs_vec;
    //bool check = parse_tfgs_for_eqcheck(function_name.get_value(), etfg_file.get_value(), ifs, &t_dst, &t_llvm, rodata_map, dst_iseq_vec, dst_insn_pcs_vec, ctx/*, false*/);
    bool llptfg_hit = false;
    for (auto const& sfname_tfg : llptfg.get_function_tfg_map()) {
      if(cur_function_name == sfname_tfg.first->call_context_get_cur_fname()->get_str()){
        llptfg_hit = true;
        break;
      }
    }
    if (!llptfg_hit) {
      cout << _FNLN_ << ": Error, could not find LLVM spec for function '" << cur_function_name << "'\n";
      exit(1);
    }
    //if (!check) {
    //  cout << "Invalid eq file" << endl;
    //  exit(1); //return 1 if does not need further exploration
    //}
    //dshared_ptr<tfg_llvm_t> t_llvm = make_dshared<tfg_llvm_t>(*llptfg.get_function_tfg_map().at(cur_function_name));
    //dshared_ptr<tfg> t_llvm = llptfg.get_function_tfg_map().at(cur_function_name);

    rodata_map_t const& rodata_map = fname_tfg.second.get_rodata_map();
    dshared_ptr<tfg> const& t_dst = fname_tfg.second.get_tfg();
    vector<dst_insn_t> const& dst_iseq_vec = fname_tfg.second.get_dst_iseq_vec();
    vector<dst_ulong> const& dst_insn_pcs_vec = fname_tfg.second.get_dst_insn_pcs_vec();

    fixed_reg_mapping_t dst_fixed_reg_mapping = fixed_reg_mapping_t::default_fixed_reg_mapping_for_function_granular_eq();
    //shared_ptr<eqspace::eqcheck> e = make_shared<eqspace::eqcheck>("eq.proof", function_name.get_value(), t_llvm, t_dst, dst_fixed_reg_mapping, rodata_map, dst_iseq_vec, dst_insn_pcs_vec, ctx, "", false, qt, true, context::XML_OUTPUT_FORMAT_TEXT_COLOR);

    //corr_graph const& cg = e->corr_graph_read_from_file(proof_file.get_value());
    ifstream is(proof_file.get_value());
    //corr_graph cg(is, "cg", e);
    dshared_ptr<cg_with_asm_annotation> cg = cg_with_asm_annotation::corr_graph_from_stream(is, ctx);

    dshared_ptr<cg_with_inductive_preds> cgi = make_dshared<cg_with_inductive_preds>(*cg);
    //DYN_DEBUG(eqcheck, cout << __func__ << " " << __LINE__ << ": checking equivalence proof\n");
    if (!cgi->check_equivalence_proof()) {
      cout << "Inductive preds check failed!\n";
      exit(1);
    }

    dshared_ptr<cg_with_relocatable_memlabels> cgr = make_dshared<cg_with_relocatable_memlabels>(*cgi);
    if (!cgr->check_equivalence_proof()) {
      dshared_ptr<eqcheck> const& e = cgr->get_eq();
      failed_cg_set_t failed_cg_set = e->get_failed_cg_set();
      list<dshared_ptr<cg_with_failcond>> const& ls = failed_cg_set.failed_cg_set_get_ls();
      ASSERT(ls.size() >= 1);
      failed_cg_set.failed_cg_set_sort();
      failed_cg_set.failed_cg_set_prune(1);
      failed_cg_set.failed_cg_set_compute_correctness_covers();
      failed_cg_set.failed_cg_set_to_stream(cout);
      cout << "Relocatable memlabels check failed!\n";
      exit(1);
    }
  }
  solver_kill();
  call_on_exit_function();
  exit(0);
}
