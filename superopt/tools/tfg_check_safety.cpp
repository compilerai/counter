#include <sys/stat.h>
#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <iostream>
#include <string>
#include <boost/filesystem.hpp>
#include <boost/range.hpp>
#include <magic.h>

#include "support/mytimer.h"
#include "support/log.h"
#include "support/cl.h"
#include "support/globals.h"
#include "support/globals_cpp.h"
#include "support/timers.h"
#include "support/read_source_files.h"
#include "support/src-defs.h"
#include "support/dyn_debug.h"

#include "expr/consts_struct.h"
#include "expr/z3_solver.h"

#include "rewrite/assembly_handling.h"

#include "tfg/tfg_llvm.h"

#include "ptfg/llptfg.h"

#include "i386/insn.h"
#include "x64/insn.h"
#include "etfg/etfg_insn.h"

#include "eq/eqcheck.h"
#include "eq/parse_input_eq_file.h"
#include "eq/corr_graph.h"
#include "eq/cg_with_asm_annotation.h"
#include "eq/function_cg_map.h"


#define DEBUG_TYPE "main"

using namespace std;

#define MAX_GUESS_LEVEL 1
void check_ub_safety(context* ctx, list<guarded_pred_t> const& gpreds, string const& ub_type, dshared_ptr<tfg_llvm_t> t_llvm, context::xml_output_format_t xml_output_format, string xml_output_file, string cur_function_name)
{
  for(auto const& gpred : gpreds)
  {
    bool guess_made_weaker;
    expr_ref inequivalence_cond = gpred.second->get_lhs_expr();
    ASSERT(gpred.second->get_rhs_expr() == expr_true(ctx));
    proof_status_t pret;
    list<counter_example_t> ret_counterexamples;
    proof_result_t proof_result = t_llvm->decide_hoare_triple_check_ub({}, pc::start(), gpred.first/*, local_sprel_expr_guesses_t()*/, aliasing_constraints_t(), predicate::mk_predicate_ref(expr_not(inequivalence_cond), "inequivalence-cond"), guess_made_weaker);
    pret = proof_result.get_proof_status();
    ret_counterexamples = proof_result.get_counterexamples();
    //proof_status_t pret = t_llvm->decide_hoare_triple_helper({}, pc::start(), gpred.first, predicate::mk_predicate_ref(expr_not(inequivalence_cond), "inequivalence-cond"), ret_counterexamples, guess_made_weaker);
    if(pret != proof_status_disproven)
      continue;

    ostringstream ss;
    ss << "Found ub-safety violation of type " << ub_type << " for function " << cur_function_name << ": "<< endl;
    ostringstream ess;
    ess << expr_xml_print_after_rename_to_source_using_tfg_llvms(inequivalence_cond, t_llvm->get_symbol_map(), t_llvm->get_locals_map(), xml_output_format, t_llvm, gpred.first->graph_edge_composition_get_to_pc(), dshared_ptr<tfg_llvm_t>::dshared_nullptr(), pc::start());
    ess << " at ";
    ess << gpred.first->graph_edge_composition_get_to_pc() << " program location. ";
    ss << ess.str();
    MSG(ss.str().c_str());
    if (xml_output_file != "") {
      g_xml_output_stream << "<ub_safety_cond>";
      g_xml_output_stream << ess.str();
      g_xml_output_stream << "</ub_safety_cond>";
    }
    size_t i = 0;
    for (auto const& ce : ret_counterexamples) {
      stringstream ss, ess;
      ss << "Counterexample #" << i << endl;
      ess << counter_example_xml_print_only_renamed_to_source_mapping(ctx, ce, t_llvm->get_symbol_map(), t_llvm->get_locals_map(), xml_output_format, t_llvm, gpred.first->graph_edge_composition_get_to_pc(), "  ");
      ss << ess.str();
      MSG(ss.str().c_str());
      if (xml_output_file != "") {
        g_xml_output_stream << "<ub_safety_cond_counterexample>";
        g_xml_output_stream << ess.str();
        g_xml_output_stream << "</ub_safety_cond_cond_counterexample>";
      }
      i++;
    }
  }
}
int main(int argc, char **argv)
{
  // command line args processing
  cl::arg<string> src_c_file(cl::implicit_prefix, "", "path to C source file");
  cl::arg<unsigned> unroll_factor(cl::explicit_prefix, "unroll-bound", 1, "Loop unrolling factor. The maximum number of times the to-pc of any edge can appear in a path (1 is implicit).");
  cl::arg<bool> disable_shiftcount(cl::explicit_flag, "disable_shiftcount", false, "Disable the modeling of undefined behaviour related to shiftcount operand, i.e., shiftcount <= log_2(bitwidth of first operand)");
  cl::arg<bool> disable_divbyzero(cl::explicit_flag, "disable_divbyzero", false, "Disable the modeling of undefined behaviour related to division operation, i.e., divisor != 0)");
  cl::arg<bool> disable_null_dereference(cl::explicit_flag, "disable_null_dereference", false, "Disable the modeling of undefined behaviour related to null-dereference, i.e., dereferenced-pointer != 0)");
  cl::arg<bool> disable_memcpy_src_alignment(cl::explicit_flag, "disable_memcpy_src_alignment", false, "Disable the modeling of undefined behaviour related to alignment of the source operand of a memcpy operation, i.e., src must be aligned to the size of the memcpy)");
  cl::arg<bool> disable_memcpy_dst_alignment(cl::explicit_flag, "disable_memcpy_dst_alignment", false, "Disable the modeling of undefined behaviour related to alignment of the destination operand of a memcpy operation, i.e., dst must be aligned to the size of the memcpy)");
  cl::arg<bool> disable_belongs_to_variable_space(cl::explicit_flag, "disable_belongs_to_variable_space", false, "Disables the generation of belongs to addressvariable space assumptions (involving inequalities)");
  cl::cl cmd("Source code analyzer: checks for safety rules violations in the given source code functions");
  cmd.add_arg(&src_c_file);
  cmd.add_arg(&disable_shiftcount);
  cmd.add_arg(&disable_divbyzero);
  cmd.add_arg(&disable_null_dereference);
  cmd.add_arg(&disable_memcpy_src_alignment);
  cmd.add_arg(&disable_memcpy_dst_alignment);
  cmd.add_arg(&disable_belongs_to_variable_space);
  cmd.add_arg(&unroll_factor);
  cl::arg<string> debug(cl::explicit_prefix, "dyn-debug", "", "Enable dynamic debugging for debug-class(es).  Expects comma-separated list of debug-classes with optional level e.g. --debug=correlate=2,smt_query=1,dumptfg,decide_hoare_triple,update_invariant_state_over_edge");
  cmd.add_arg(&debug);
  cl::arg<string> xml_output_format_str(cl::explicit_prefix, "xml-output-format", "text-color", "xml output format to use during xml printing [html|text-color|text-nocolor]");
  cmd.add_arg(&xml_output_format_str);

  cl::arg<string> xml_output_file(cl::explicit_prefix, "xml-output", "", "Dump XML output to this file");
  cmd.add_arg(&xml_output_file);
  cl::arg<string> stdout_(cl::explicit_prefix, "stdout", "", "Redirect stdout to this filename");
  cmd.add_arg(&stdout_);
  cl::arg<string> stderr_(cl::explicit_prefix, "stderr", "", "Redirect stderr to this filename");
  cmd.add_arg(&stderr_);
  cl::arg<string> tmpdir_path(cl::explicit_prefix, "tmpdir-path", "", "Include path for C source compilation");
  cmd.add_arg(&tmpdir_path);
  CPP_DBG_EXEC(ARGV_PRINT,
      for (int i = 0; i < argc; i++) {
        cout << "argv[" << i << "] = " << argv[i] << endl;
      }
  );
  cmd.parse(argc, argv);
  context::xml_output_format_t xml_output_format = context::xml_output_format_from_string(xml_output_format_str.get_value());
  //cout << _FNLN_ << ": xml_output_format_str = " << xml_output_format_str.get_value() << endl;

  if (stdout_.get_value() != "") {
    freopen(stdout_.get_value().c_str(), "w", stdout);
  }
  if (stderr_.get_value() != "") {
    freopen(stderr_.get_value().c_str(), "w", stderr);
  }

  if (xml_output_file.get_value() != "") {
    g_xml_output_stream.open(xml_output_file.get_value());
    ASSERT(g_xml_output_stream.is_open());
    g_xml_output_stream << "<Souce code analyzer>";
  }


  init_dyn_debug_from_string(debug.get_value());
  CPP_DBG_EXEC(DYN_DEBUG, print_debug_class_levels());
  g_query_dir_init();

  g_ctx_init();
  solver_init();
  src_init();
  dst_init();
  
  context::config cfg(3600, 3600);
  cfg.unroll_factor = unroll_factor.get_value();
  context *ctx = g_ctx;
  ctx->set_config(cfg);
  ctx->parse_consts_db(CONSTS_DB_FILENAME);
  consts_struct_t &cs = ctx->get_consts_struct();


  set<string> undef_behaviour_to_be_ignored;
  if (disable_shiftcount.get_value()) {
    undef_behaviour_to_be_ignored.insert(UNDEF_BEHAVIOUR_ASSUME_ISSHIFTCOUNT);
  }
  if (disable_divbyzero.get_value()) {
    undef_behaviour_to_be_ignored.insert(UNDEF_BEHAVIOUR_ASSUME_DIVBYZERO);
  }
  if (disable_null_dereference.get_value()) {
    undef_behaviour_to_be_ignored.insert(UNDEF_BEHAVIOUR_ASSUME_DEREFERENCE);
  }
  if (disable_memcpy_src_alignment.get_value()) {
    undef_behaviour_to_be_ignored.insert(UNDEF_BEHAVIOUR_ASSUME_ALIGN_MEMCPY_SRC);
  }
  if (disable_memcpy_dst_alignment.get_value()) {
    undef_behaviour_to_be_ignored.insert(UNDEF_BEHAVIOUR_ASSUME_ALIGN_MEMCPY_DST);
  }
  if (disable_belongs_to_variable_space.get_value()) {
    undef_behaviour_to_be_ignored.insert(UNDEF_BEHAVIOUR_ASSUME_BELONGS_TO_VARIABLE_SPACE);
  }

  {
    string msg = "Sorce code analysis at unroll factor " + int_to_string(cfg.unroll_factor);
    MSG(msg.c_str());
  }
  string etfg_filename = src_c_file.get_value();

  bool model_llvm_semantics = false;
  etfg_filename = convert_source_file_to_etfg(etfg_filename, xml_output_file.get_value(), xml_output_format_str.get_value(), "ALL", model_llvm_semantics, 1, "", tmpdir_path.get_value());
  string msg = "Sorce code analysis at unroll factor " + int_to_string(cfg.unroll_factor);
  MSG(msg.c_str());

  ifstream in_llvm(etfg_filename);
  if (!in_llvm.is_open()) {
    cout << __func__ << " " << __LINE__ << ": Could not open etfg_filename '" << etfg_filename << "'\n";
  }
  ASSERT(in_llvm.is_open());
  llptfg_t llptfg(in_llvm, ctx);
  in_llvm.close();

  auto const& function_tfg_map = llptfg.get_function_tfg_map();

  set<string> failing_functions;
  for (auto const& fname_tfg : function_tfg_map) {
    string const& cur_function_name = fname_tfg.first->call_context_get_cur_fname()->get_str();
    //dshared_ptr<tfg_llvm_t> t_llvm = make_dshared<tfg_llvm_t>(*fname_tfg.second);
    dshared_ptr<tfg> t = fname_tfg.second;
    dshared_ptr<tfg_llvm_t> t_llvm = dynamic_pointer_cast<tfg_llvm_t>(t);
    ASSERT(t_llvm);

    predicate_set_t input_assumes;
    t_llvm->tfg_preprocess_before_eqcheck(/*undef_behaviour_to_be_ignored, */input_assumes, /*populate_suffixpaths*/ false);
    if (t_llvm->contains_float_op() ) {
      cout << "ERROR_contains_float_op" << endl;
      return 0;
    }
    if (t_llvm->contains_unsupported_op()) {
      cout << "ERROR_contains_unsupported_op" << endl;
      return 0;
    }

    ctx->clear_cache();

    auto chrono_start = chrono::system_clock::now();
    stats::get().clear();

/* TODO add code */
  pc from_pc = pc::start();
  int from_pc_subsubindex = from_pc.get_subsubindex();
  expr_ref incident_fcall_nextpc = t_llvm->get_incident_fcall_nextpc(from_pc);


  list<shared_ptr<tfg_full_pathset_t const> > pths;

  map<pair<pc,int>, shared_ptr<tfg_full_pathset_t const> > tfg_paths_to_pcs = t_llvm->get_unrolled_paths_from(tfg_enum_props_t::src_tfg_enum_props, from_pc, from_pc_subsubindex, incident_fcall_nextpc, cfg.unroll_factor);

  for(auto const& pth : tfg_paths_to_pcs)
  {
    if(pth.first.first.is_exit()) {
      ASSERT(pth.second);
      pths.push_back(pth.second);
    }
  }
  for(auto const& fps : pths)
  {
    auto gpreds = t_llvm->get_potentially_zero_divisors(fps->get_graph_ec());
    check_ub_safety(ctx, gpreds, "divbyzero", t_llvm, xml_output_format, xml_output_file.get_value(), cur_function_name);
    auto gpreds1 = t_llvm->get_potentially_out_of_bound_memory_accesses(fps->get_graph_ec());
    check_ub_safety(ctx, gpreds1, "outofboundmem", t_llvm, xml_output_format, xml_output_file.get_value(), cur_function_name);
  }
  unsigned num_smt_queries = stats::get().get_counter_value("smt-queries");
  auto chrono_now = chrono::system_clock::now();
  auto elapsed = chrono_now - chrono_start;
  //cout << (cur_eq_result ? (inequivalence_cond ? "INEQ" : "EQUIV") : "FAIL") << " equivalence check for function " << cur_function_name << ". " << chrono_duration_to_string(elapsed) << "s, " << /*num_smt_queries << "smtq" << ", " <<*/ stats::get().get_timer("query:smt")->to_string() << ", " << stats::get().get_timer("decide_hoare_triple.ce")->to_string() << ", " << stats::get().get_timer("decide_hoare_triple_helper")->to_string() << endl;
  // if (!cur_eq_result || inequivalence_cond) {
  //   eq_result = false;
  //   failed_functions.insert(cur_function_name);
  //   if (stop_on_first_failure.get_value()) {
  //     break;
  //   }
  // } else {
  //   fname_cg_map.insert(make_pair(cur_function_name, cur_eq_result));
  //   ostringstream ss;
  //   ss << "Successfully computed equivalence for function: " << cur_function_name << ".\n";
  //   MSG(ss.str().c_str());
  // }
  }

  //solver_kill(); // DO NOT UNCOMMENT IF YOU WANT TO GET SOLVER STATS
  call_on_exit_function();

  if (g_xml_output_stream.is_open()) {
    g_xml_output_stream << "</Souce code analyzer>";
    g_xml_output_stream.close();
  }
  return 0;
}
