#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include "eq/eqcheck.h"
#include "tfg/parse_input_eq_file.h"
#include "expr/consts_struct.h"
#include "support/mytimer.h"
#include "support/log.h"
#include "support/cl.h"
#include "support/globals.h"
#include "eq/corr_graph.h"
#include "support/globals_cpp.h"
#include "expr/z3_solver.h"
#include "support/timers.h"
#include "tfg/tfg_llvm.h"

#include "i386/insn.h"
#include "codegen/etfg_insn.h"
#include "support/src-defs.h"
#include "rewrite/assembly_handling.h"
#include "eq/cg_with_asm_annotation.h"
#include "eq/function_cg_map.h"
#include "support/dyn_debug.h"

#include <iostream>
#include <string>
#include <boost/filesystem.hpp>
#include <boost/range.hpp>
#include <magic.h>

#define DEBUG_TYPE "main"

using namespace std;

#define MAX_GUESS_LEVEL 1

static void
skip_till_next_function(ifstream &in)
{
  string line;
  do {
    if (!getline(in, line)) {
      cout << __func__ << " " << __LINE__ << ": reached end of file\n";
      return;
    }
  } while (!string_has_prefix(line, FUNCTION_FINISH_IDENTIFIER));
  return;
}

static shared_ptr<corr_graph const>
genEqProof(string const &proof_file, string const &function_name, shared_ptr<tfg> t_llvm, shared_ptr<tfg> t_dst, rodata_map_t const &rodata_map, vector<dst_insn_t> const& dst_iseq_vec, vector<dst_ulong> const& dst_insn_pcs_vec, context *ctx, bool llvm2ir, set<string> const &undef_behaviour_to_be_ignored, string const &assume_file, string const& local_sprel_assumes_file, string const &check_filename, quota const &qt)
{
  consts_struct_t &cs = ctx->get_consts_struct();
  //map<symbol_id_t, src_ulong> rodata_map;

  ostringstream ss;
  ss << "Computing equivalence for function: " << function_name << endl;
  MSG(ss.str().c_str());

  //t_llvm->substitute_symbols_using_rodata_map(rodata_map);
  //CPP_DBG_EXEC(EQCHECK, cout << __func__ << " " << __LINE__ << ": SRC\n" << t_llvm->graph_to_string() << endl);
  //t_llvm->substitute_rodata_accesses(*t_dst);

  predicate_set_t input_assumes;
  if (!assume_file.empty()) {
    input_assumes = read_assumes_from_file(assume_file, ctx);
  }
  t_dst->tfg_preprocess_before_eqcheck(undef_behaviour_to_be_ignored, input_assumes, /*populate_suffixpaths_and_pred_avail_exprs*/false);
  //t_llvm->graph_add_dst_rodata_symbol_mask_locs_on_src_mem(t_dst->get_symbol_map());
  t_llvm->set_symbol_map(t_dst->get_symbol_map());
  t_llvm->tfg_preprocess_before_eqcheck(undef_behaviour_to_be_ignored, input_assumes, /*populate_suffixpaths_and_pred_avail_exprs*/true);
  //t_llvm->remove_assume_preds_with_comments(undef_behaviour_to_be_ignored);
  //t_dst->remove_assume_preds_with_comments(undef_behaviour_to_be_ignored);

  assert(t_llvm->get_argument_regs().size() == t_dst->get_argument_regs().size() && "args size not equal");

  list<shared_ptr<tfg_node>> llvm_nodes, dst_nodes;
  t_llvm->get_nodes(llvm_nodes);
  t_dst->get_nodes(dst_nodes);
  for (auto ln : llvm_nodes) {
    pc const &p = ln->get_pc();
    if (!p.is_exit()) {
      continue;
    }
    ASSERT(t_dst->find_node(p) != NULL);
    assert(t_llvm->get_return_regs_at_pc(p).size() == t_dst->get_return_regs_at_pc(p).size() && "return-regs size not equal");
  }
  /*if (!disable_uninitialized_variables.get_value()) {
    map<pc, map<graph_loc_id_t, bool>> init_status;
    list<pc> exit_pcs;
    init_status = t_llvm->determine_initialization_status_for_locs();
    t_llvm->get_exit_pcs(exit_pcs);
    for (auto exit_pc : exit_pcs) {
      if (t_llvm->return_register_is_uninit_at_exit(G_LLVM_RETURN_REGISTER_NAME, exit_pc, init_status)) {
        t_llvm->eliminate_return_reg_at_exit(G_LLVM_RETURN_REGISTER_NAME, exit_pc);
        stringstream ss;
        int eax_regnum = 5;
        ss << G_LLVM_RETURN_REGISTER_NAME << I386_EXREG_GROUP_GPRS << "." << eax_regnum;
        t_dst->eliminate_return_reg_at_exit(ss.str(), exit_pc);
      }
    }
  }*/

  if (t_llvm->contains_float_op() || t_dst->contains_float_op()) {
    cout << "ERROR_contains_float_op" << endl;
    return nullptr;
  }
  if (t_llvm->contains_unsupported_op() || t_dst->contains_unsupported_op()) {
    cout << "ERROR_contains_unsupported_op" << endl;
    return nullptr;
  }
  if(t_llvm->contains_backedge() != t_dst->contains_backedge())
  {
    cout << "ERROR_src-loop-xor-dst-loop" << endl;
    return nullptr;
  }

  //t_llvm->truncate_function_arguments_using_tfg(*t_dst);
  //cs.set_argument_constants(t_llvm->get_argument_regs());

  //t_llvm->model_nop_callee_summaries(t_llvm->get_callee_summaries());
  //t_dst->model_nop_callee_summaries(t_llvm->get_callee_summaries());

  //t_llvm->resize_farg_exprs_for_function_calling_conventions();
  //if (llvm2ir) {
  //  t_dst->resize_farg_exprs_for_function_calling_conventions();
  //}

  fixed_reg_mapping_t dst_fixed_reg_mapping = fixed_reg_mapping_t::default_fixed_reg_mapping_for_function_granular_eq();

  shared_ptr<tfg_llvm_t> tssa = dynamic_pointer_cast<tfg_llvm_t>(t_llvm);
  ASSERT(tssa);
  tssa->tfg_llvm_populate_varname_definedness();

  local_sprel_expr_guesses_t lsprel_assumes;
  if (!local_sprel_assumes_file.empty()) {
    ifstream fin(local_sprel_assumes_file);
    if (!fin.good()) {
      cout << _FNLN_ << ": failed to open/read " << local_sprel_assumes_file << endl;
      assert(false);
    }
    lsprel_assumes.local_sprel_expr_guesses_from_stream(fin, ctx);
    DYN_DEBUG(local_sprel_expr, cout << _FNLN_ << ": input lsprel assumes: " << lsprel_assumes.to_string() << endl);
  }

  shared_ptr<eqspace::eqcheck> e = make_shared<eqspace::eqcheck>(proof_file, function_name, t_llvm, t_dst, dst_fixed_reg_mapping, rodata_map, dst_iseq_vec, dst_insn_pcs_vec, ctx/*, relevant_memlabels*/, llvm2ir, qt);
  /*, cg_no_fcall_side_effects, cg_acyclic, cg_acyclic_no_fcall_side_effects*/;
  //bool res = false, res_no_fcall_side_effects = false;
  //bool res_acyclic = false, res_acyclic_no_fcall_side_effects = false;

  //cout << __func__ << " " << __LINE__ << ": t_llvm = " << t_llvm->graph_to_string() << endl;

  shared_ptr<corr_graph const> cg = eqcheck::check_eq(e, lsprel_assumes, check_filename);
  stats::get().set_flag("timeout-occured", ctx->timeout_occured());
  //cout << __func__ << " " << __LINE__ << ": res = " << res << endl;
  /*if (!cg && !disable_remove_fcall_side_effects.get_value()) {
    t_llvm->remove_fcall_side_effects();
    t_dst->remove_fcall_side_effects();
    e.clear_fcall_side_effects();
    cout << __func__ << __LINE__ << ": Trying after removing fcall side effects." << endl;
    //cout << "t_llvm:\n" << t_llvm.graph_to_string() << endl;
    //cout << "t_dst:\n" << t_dst.graph_to_string() << endl;
    //cout << "t_dst done:\n";
    cg_no_fcall_side_effects = e.check_eq(relevant_memlabels);
    //cout << __func__ << " " << __LINE__ << ": res_no_fcall_side_effects = " << res_no_fcall_side_effects << endl;
    if (cg_no_fcall_side_effects) {
      cout << "Provable CG no_fcall_side_effects:\n" << cg_no_fcall_side_effects->provable_cg_to_string() << "\ndone Provable CG\n";
      cg_no_fcall_side_effects->write_provable_cg_to_proof_file(proof_file.get_value(), true, false);
    } else {
      cg_no_fcall_side_effects->write_provable_cg_to_proof_file(proof_file.get_value(), false, false);
    }
  }*/
  if (cg) {
    //cout << "Provable CG:\n" << cg->provable_cg_to_string() << "\ndone Provable CG\n";
    cg->print_corr_counters_stat();
  }
  CPP_DBG_EXEC(STATS,
    cout << "Printing statistics --\n";
    //print_all_timers();
    cout << stats::get();
    //cout << ctx->stat() << endl;
  );

  //if (cg && annotate_assembly_file != "") {
  //  shared_ptr<cg_with_asm_annotation> cg_asm_annot = make_shared<cg_with_asm_annotation>(*cg, annotate_assembly_file);
  //  ofstream ofs(annotate_assembly_file_output);
  //  ASSERT(ofs.is_open());
  //  cg_asm_annot->cg_remove_marker_calls_and_gen_annotated_assembly(ofs);
  //  ofs.close();
  //}

  return cg;
  //if(cg) return true;
  //else return false;
}

int main(int argc, char **argv)
{
  autostop_timer func_timer(__func__);
  // command line args processing
  cl::arg<string> etfg_file(cl::implicit_prefix, "", "path to .bc/.etfg file");
  cl::arg<string> tfg_file(cl::implicit_prefix, "", "path to .o (or .tfg) file");
  cl::arg<string> function_name(cl::explicit_prefix, "f", "ALL", "function name to be compared for equivalence");
  cl::arg<string> record_filename(cl::explicit_prefix, "record", "", "generate record log of SMT queries to given filename");
  cl::arg<string> replay_filename(cl::explicit_prefix, "replay", "", "replay log of SMT queries from given filename");
  cl::arg<string> check_filename(cl::explicit_prefix, "check", "", "Check proof (do not generate proof) stored in the argument (filename)");
  cl::arg<string> solver_stats_filename(cl::explicit_prefix, "solver_stats", "", "save smt solver statistics to given filename");
  cl::arg<unsigned> page_size(cl::explicit_prefix, "page_size", 4096, "Page size used during safety check");
  cl::arg<unsigned> unroll_factor(cl::explicit_prefix, "unroll-factor", 1, "Loop unrolling factor. The maximum number of times the to-pc of any edge can appear in a path (1 is implicit).");
  cl::arg<unsigned> dst_unroll_factor(cl::explicit_prefix, "dst-unroll-factor", 1, "Dst graph loop unrolling factor");
  cl::arg<bool> cleanup_query_files(cl::explicit_flag, "cleanup-query-files", false, "Cleanup all query files like is_expr_equal_using_lhs_set, etc., on pass result");
  cl::arg<bool> stop_on_first_failure(cl::explicit_flag, "stop-on-first-failure", false, "Stop on first proof failure (for any single function)");
  cl::arg<unsigned> smt_query_timeout(cl::explicit_prefix, "smt-query-timeout", 60, "Timeout per query (s)");
  cl::arg<unsigned> sage_query_timeout(cl::explicit_prefix, "sage-query-timeout", 600, "Timeout per query (s)");
  cl::arg<unsigned> global_timeout(cl::explicit_prefix, "global-timeout", 36000, "Total timeout (s)");
  cl::arg<unsigned> memory_threshold_gb(cl::explicit_prefix, "memory-threshold", 128, "Memory threshold (in GiB)");
  cl::arg<unsigned long> max_address_space_size(cl::explicit_prefix, "max-address-space-size", (unsigned long)-1, "Max address space size");
  cl::arg<string> proof_file(cl::explicit_prefix, "proof", "", "File to write proof");
  //cl::arg<string> existing_proof_file(cl::explicit_prefix, "existing-proof", "", "File in which a partial (or total) existing proof already exists; proof is attempted for only thos functions that are not already present in this file.");
  cl::arg<string> annotate_assembly_file(cl::explicit_prefix, "annotate-assembly", "", "Assembly file to annotate");
  cl::arg<string> annotate_assembly_file_output(cl::explicit_prefix, "annotate-assembly-output", "", "Filename of the output annotated assembly file");
  cl::arg<string> harvest_output_file(cl::explicit_prefix, "harvest", "", "File to write harvest output");
  cl::arg<string> harvest_log_file(cl::explicit_prefix, "harvest_log", "", "File to write harvest log");
  //cl::arg<bool> force_acyclic(cl::explicit_flag, "force_acyclic", false, "Collapse the TFGs so that acyclic TFGs are reduced to a single edge");
  //cl::arg<bool> test_acyclic(cl::explicit_flag, "test_acyclic", false, "Test equivalence of acyclic TFGs against collapsed TFGs");
  cl::arg<string> assume_file(cl::explicit_prefix, "assume", "", "File to read assume clauses from (for debugging)");
  cl::arg<string> local_sprel_assumes_file(cl::explicit_prefix, "local_sprel_assumes", "", "File to read assumed local-sprel mappings from");
  cl::arg<bool> disable_shiftcount(cl::explicit_flag, "disable_shiftcount", false, "Disable the modeling of undefined behaviour related to shiftcount operand, i.e., shiftcount <= log_2(bitwidth of first operand)");
  cl::arg<bool> disable_divbyzero(cl::explicit_flag, "disable_divbyzero", false, "Disable the modeling of undefined behaviour related to division operation, i.e., divisor != 0)");
  cl::arg<bool> disable_null_dereference(cl::explicit_flag, "disable_null_dereference", false, "Disable the modeling of undefined behaviour related to null-dereference, i.e., dereferenced-pointer != 0)");
  cl::arg<bool> disable_memcpy_src_alignment(cl::explicit_flag, "disable_memcpy_src_alignment", false, "Disable the modeling of undefined behaviour related to alignment of the source operand of a memcpy operation, i.e., src must be aligned to the size of the memcpy)");
  cl::arg<bool> disable_memcpy_dst_alignment(cl::explicit_flag, "disable_memcpy_dst_alignment", false, "Disable the modeling of undefined behaviour related to alignment of the destination operand of a memcpy operation, i.e., dst must be aligned to the size of the memcpy)");
  cl::arg<bool> disable_belongs_to_variable_space(cl::explicit_flag, "disable_belongs_to_variable_space", false, "Disables the generation of belongs to addressvariable space assumptions (involving inequalities)");
  //cl::arg<bool> disable_uninitialized_variables(cl::explicit_flag, "disable_uninitialized_variables", false, "Disables the modeling of undefined behaviour related to uninitialized variables");
  cl::arg<bool> disable_correl_locals(cl::explicit_flag, "disable_correl_locals", false, "Disables the correlation of local variables to stack addresss");
  cl::arg<bool> disable_remove_fcall_side_effects(cl::explicit_flag, "disable_remove_fcall_side_effects", false, "Disables the attempt to compute equivalence after removing function call side effects on memory");
  cl::arg<string> result_suffix(cl::explicit_prefix, "result_suffix", "", "suffix to use for reporting results on stdout");
  cl::arg<bool> correl_locals_try_all_possibilities(cl::explicit_flag, "correl_locals_try_all_possibilities", false, "Try all possibilities for correlating locals to stack addresses while discharging a proof obligation");
  cl::arg<bool> correl_locals_consider_all_local_variables(cl::explicit_flag, "correl_locals_consider_all_local_variables", false, "Consider all local variables to compute potential local-sprel_expr guesses (and not just address taken local variables)");
  cl::arg<bool> correl_locals_brute_force(cl::explicit_flag, "correl_locals_brute_force", false, "Brute force correlation algo: for each possible local-sprel correlation, rerun the entire correlation procedure");
  cl::arg<bool> disable_cache(cl::explicit_flag, "disable_cache", false, "Disable context cache");
  cl::arg<bool> use_only_live_src_vars(cl::explicit_flag, "use_only_live_src_vars", false, "Select whether to only use live src locs or all.  Default: false (all)");
  cl::arg<bool> use_sage(cl::explicit_flag, "use_sage", false, "Select whether to use sage for solving BV equations or C++ implementation.  Default: false (Use C++ implementation)");
  //cl::arg<unsigned> sage_dedup_threshold(cl::explicit_prefix, "sage_dedup_threshold", 20, "Set bv_solve_sage dedup column threshold.  Default: 20");
  cl::arg<unsigned> warm_start_param(cl::explicit_prefix, "warm_start_param", 2, "The number of CE to start invariant inference");
  cl::arg<int> solver_config(cl::explicit_prefix, "solver_config", 0, "Which solvers to use; 0- both, 1- yices only 2- z3 only");
  cl::arg<int> max_lookahead(cl::explicit_prefix, "max_lookahead", 0, "Max lookahead value.  Valid range: [0, GUESSING_MAX_LOOKAHEAD)");
  //cl::arg<bool> disable_heuristic_pruning(cl::explicit_flag, "disable_hpruning", false, "Disable heuristic pruning cache");
  cl::arg<bool> disable_pernode_CE_cache(cl::explicit_flag, "disable_pernode_CE_cache", false, "Disable per node CE caching for invariant inference");
  cl::arg<bool> enable_local_sprel_guesses(cl::explicit_flag, "enable_local_sprel_guesses", false, "Enable local sprel inference");
  cl::arg<bool> add_mod_exprs(cl::explicit_flag, "add_mod_exprs", false, "Add mod expressions required for checking vectorization");
  cl::arg<bool> llvm2ir(cl::explicit_flag, "llvm2ir", false, "Check one LLVM program against another");
  cl::arg<bool> disable_points_propagation(cl::explicit_flag, "disable_points_propagation", false, "disable propagation of points across edges");
  cl::arg<bool> disable_inequality_inference(cl::explicit_flag, "disable_inequality_inference", false, "Disable inequality inference");
  cl::arg<bool> disable_query_decomposition(cl::explicit_flag, "disable_query_decomposition", false, "Disable Query decomposition");
  cl::arg<bool> disable_residual_loop_unroll(cl::explicit_flag, "disable_residual_loop_unroll", false, "Disable residual loop paths generation");
  cl::arg<bool> disable_loop_path_exprs(cl::explicit_flag, "disable_loop_path_exprs", false, "Disable loop path src expressions at lookahead 0");
  cl::arg<bool> disable_epsilon_src_paths(cl::explicit_flag, "disable_epsilon_src_paths", false, "Disable epsilon src path enumeration");
  cl::arg<bool> disable_dst_bv_rank(cl::explicit_flag, "disable_dst_bv_rank", false, "Disable dest preds based bv ranking");
  cl::arg<bool> disable_src_bv_rank(cl::explicit_flag, "disable_src_bv_rank", false, "Disable src preds based bv ranking");
  cl::arg<bool> disable_propagation_based_pruning(cl::explicit_flag, "disable_propagation_based_pruning", false, "Disable propagation based pruning");
  cl::arg<bool> disable_all_static_heuristics(cl::explicit_flag, "disable_all_static_heuristics", false, "Disable static heuristics");
  cl::arg<bool> choose_longest_delta_first(cl::explicit_flag, "choose_longest_delta_first", false, "Choose longest delta path first");
  cl::arg<bool> choose_shortest_path_first(cl::explicit_flag, "choose_shortest_path_first", false, "Choose shortest path length for same delta fist");
  cl::arg<bool> anchor_loop_tail(cl::explicit_flag, "anchor_loop_tail", false, "Use loop tails as anchor nodes.  Loop head is used as default");
  cl::arg<bool> consider_non_vars_for_dst_ineq(cl::explicit_flag, "consider_non_vars_for_dst_ineq", false, "Consider non-vars (stack slots) for dst inequality inference.  For performance reasons, default is false");
  cl::arg<string> debug(cl::explicit_prefix, "debug", "", "Enable dynamic debugging for debug-class(es).  Expects comma-separated list of debug-classes with optional level e.g. --debug=correlate=2,smt_query=1,dumptfg,decide_hoare_triple,update_invariant_state_over_edge");

  cl::cl cmd("Equivalence checker: checks equivalence of given .eq file");
  cmd.add_arg(&function_name);
  cmd.add_arg(&record_filename);
  cmd.add_arg(&replay_filename);
  cmd.add_arg(&check_filename);
  cmd.add_arg(&solver_stats_filename);
  cmd.add_arg(&page_size);
  cmd.add_arg(&etfg_file);
  cmd.add_arg(&tfg_file);
  cmd.add_arg(&result_suffix);
  cmd.add_arg(&unroll_factor);
  cmd.add_arg(&dst_unroll_factor);
  cmd.add_arg(&cleanup_query_files);
  cmd.add_arg(&stop_on_first_failure);
  cmd.add_arg(&smt_query_timeout);
  cmd.add_arg(&sage_query_timeout);
  cmd.add_arg(&global_timeout);
  cmd.add_arg(&memory_threshold_gb);
  cmd.add_arg(&max_address_space_size);
  cmd.add_arg(&proof_file);
  //cmd.add_arg(&existing_proof_file);
  cmd.add_arg(&annotate_assembly_file);
  cmd.add_arg(&annotate_assembly_file_output);
  cmd.add_arg(&harvest_output_file);
  cmd.add_arg(&harvest_log_file);
  //cmd.add_arg(&test_acyclic);
  //cmd.add_arg(&force_acyclic);
  cmd.add_arg(&assume_file);
  cmd.add_arg(&local_sprel_assumes_file);
  cmd.add_arg(&disable_shiftcount);
  cmd.add_arg(&disable_divbyzero);
  cmd.add_arg(&disable_null_dereference);
  cmd.add_arg(&disable_memcpy_src_alignment);
  cmd.add_arg(&disable_memcpy_dst_alignment);
  cmd.add_arg(&disable_belongs_to_variable_space);
  //cmd.add_arg(&disable_uninitialized_variables);
  cmd.add_arg(&disable_correl_locals);
  cmd.add_arg(&disable_remove_fcall_side_effects);
  cmd.add_arg(&correl_locals_try_all_possibilities);
  cmd.add_arg(&correl_locals_consider_all_local_variables);
  cmd.add_arg(&correl_locals_brute_force);
  cmd.add_arg(&disable_cache);
  cmd.add_arg(&use_only_live_src_vars);
  cmd.add_arg(&use_sage);
  //cmd.add_arg(&sage_dedup_threshold);
  cmd.add_arg(&warm_start_param);
  cmd.add_arg(&solver_config);
  cmd.add_arg(&max_lookahead);
  //cmd.add_arg(&disable_heuristic_pruning);
  cmd.add_arg(&disable_pernode_CE_cache);
  cmd.add_arg(&add_mod_exprs);
  cmd.add_arg(&enable_local_sprel_guesses);
  cmd.add_arg(&llvm2ir);
  cmd.add_arg(&disable_points_propagation);
  cmd.add_arg(&disable_inequality_inference);
  cmd.add_arg(&disable_query_decomposition);
  cmd.add_arg(&disable_residual_loop_unroll);
  cmd.add_arg(&disable_loop_path_exprs);
  cmd.add_arg(&disable_epsilon_src_paths);
  cmd.add_arg(&disable_dst_bv_rank);
  cmd.add_arg(&disable_src_bv_rank);
  cmd.add_arg(&disable_propagation_based_pruning);
  cmd.add_arg(&disable_all_static_heuristics);
  cmd.add_arg(&choose_longest_delta_first);
  cmd.add_arg(&choose_shortest_path_first);
  cmd.add_arg(&anchor_loop_tail);
  cmd.add_arg(&consider_non_vars_for_dst_ineq);
  cmd.add_arg(&debug);
  cl::arg<bool> progress(cl::explicit_flag, "progress", false, "Print progress");
  cmd.add_arg(&progress);

  CPP_DBG_EXEC(ARGV_PRINT,
      for (int i = 0; i < argc; i++) {
        cout << "argv[" << i << "] = " << argv[i] << endl;
      }
  );
  cmd.parse(argc, argv);

  progress_flag = progress.get_value();

  cout << __func__ << " " << __LINE__ << ": progress_flag = " << int(progress_flag) << endl;

  string proof_filename = proof_file.get_value();
  if (proof_filename == "") {
    proof_filename = "eq.proof." + function_name.get_value();
  }

  string annotate_assembly_output_filename = annotate_assembly_file_output.get_value();
  if (annotate_assembly_file.get_value() != "" && annotate_assembly_output_filename == "") {
    annotate_assembly_output_filename = annotate_assembly_file.get_value() + ".annotated.s";
  }

  ASSERT(check_filename.get_value() != proof_filename && "Proof file (output proof) and check file (input proof) cannot be the same!\n");
  g_page_size_for_safety_checks = page_size.get_value();

  eqspace::init_dyn_debug_from_string(debug.get_value());
  CPP_DBG_EXEC(DYN_DEBUG, eqspace::print_debug_class_levels());
  g_query_dir_init();
  src_init();
  dst_init();

  if (correl_locals_try_all_possibilities.get_value()) {
    g_correl_locals_try_all_possibilities = true;
  }
  if (correl_locals_consider_all_local_variables.get_value()) {
    g_correl_locals_consider_all_local_variables = true;
  }

  quota qt(unroll_factor.get_value(), smt_query_timeout.get_value(), sage_query_timeout.get_value(), global_timeout.get_value(), memory_threshold_gb.get_value(), max_address_space_size.get_value());

  context::config cfg(qt.get_smt_query_timeout(), qt.get_sage_query_timeout());
  if (disable_cache.get_value())
    cfg.DISABLE_CACHES = true;
  if (use_only_live_src_vars.get_value())
    cfg.use_only_live_src_vars = true;
  if (use_sage.get_value())
    cfg.use_sage= true;
  //if (disable_heuristic_pruning.get_value())
  //  cfg.disable_heuristic_pruning = true;
  if (disable_pernode_CE_cache.get_value())
    cfg.disable_pernode_CE_cache = true;
  if (add_mod_exprs.get_value())
    cfg.add_mod_exprs= true;
  if (enable_local_sprel_guesses.get_value())
    cfg.enable_local_sprel_guesses = true;
  if (disable_points_propagation.get_value())
    cfg.disable_points_propagation = true;
  if (disable_inequality_inference.get_value())
    cfg.disable_inequality_inference = true;
  if (disable_query_decomposition.get_value())
    cfg.disable_query_decomposition = true;
  if (disable_residual_loop_unroll.get_value())
    cfg.disable_residual_loop_unroll = true;
  if (disable_loop_path_exprs.get_value())
    cfg.disable_loop_path_exprs = true;
  if (disable_epsilon_src_paths.get_value())
    cfg.disable_epsilon_src_paths = true;
  if (disable_dst_bv_rank.get_value())
    cfg.disable_dst_bv_rank = true;
  if (disable_src_bv_rank.get_value())
    cfg.disable_src_bv_rank = true;
  if (disable_propagation_based_pruning.get_value())
    cfg.disable_propagation_based_pruning = true;
  if (disable_all_static_heuristics.get_value())
    cfg.disable_all_static_heuristics = true;
  if (choose_longest_delta_first.get_value())
    cfg.choose_longest_delta_first= true;
  if (choose_shortest_path_first.get_value())
    cfg.choose_shortest_path_first= true;
  if (anchor_loop_tail.get_value())
    cfg.anchor_loop_tail = true;
  if (consider_non_vars_for_dst_ineq.get_value())
    cfg.consider_non_vars_for_dst_ineq = true;

  //cfg.sage_dedup_threshold = sage_dedup_threshold.get_value();
  cfg.warm_start_param = warm_start_param.get_value();
  cfg.solver_config = solver_config.get_value();
  cfg.max_lookahead = max_lookahead.get_value();
  cfg.unroll_factor = unroll_factor.get_value();
  cfg.dst_unroll_factor = dst_unroll_factor.get_value();

  g_ctx_init();
  solver_init(record_filename.get_value(), replay_filename.get_value(), solver_stats_filename.get_value());
  context *ctx = g_ctx;
  ctx->set_config(cfg);
  //consts_struct_t cs;
  //cs.parse_consts_db(&ctx);
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

  string etfg_filename = etfg_file.get_value();

  auto handle = ::magic_open(MAGIC_NONE|MAGIC_COMPRESS);
  ::magic_load(handle, NULL);

  boost::filesystem::path etfg_filepath;
  try {
    etfg_filepath = boost::filesystem::canonical(etfg_filename);
  } catch(...) {
    cout << "File '" << etfg_filename << "' does not exist\n";
    exit(1);
  }
  auto type = ::magic_file(handle, etfg_filepath.native().c_str());
  if (type) {
    stringstream ss;
    ss << type;
    string typeName = ss.str();
    //cout << "typeName = " << typeName << endl;
    if (typeName == "LLVM IR bitcode") {
      CPP_DBG_EXEC(EQCHECK, cout << "Calling llvm2tfg on '" << typeName << "' file " << etfg_filename << "'\n");
      MSG("Converting LLVM IR bitcode to Transfer Function Graph (TFG)...");
      stringstream ss;
      string new_etfg_filename = (etfg_filename + ".etfg");
      ss << SUPEROPT_INSTALL_DIR << "/bin/llvm2tfg -f " << function_name.get_value() << " " << etfg_filename << " -o " << new_etfg_filename;
      cout << "cmd = " << ss.str() << endl;
      xsystem(ss.str().c_str());
      CPP_DBG_EXEC(EQCHECK, cout << "done Calling llvm2tfg on '" << typeName << "' file " << etfg_filename << "' to produce '" << new_etfg_filename << "'\n");
      etfg_filename = new_etfg_filename;
    }
  }

  string tfg_filename = tfg_file.get_value();
  ASSERT(tfg_filename != "");
  boost::filesystem::path tfg_filepath;
  try {
    tfg_filepath = boost::filesystem::canonical(tfg_filename);
  } catch(...) {
    cout << "File '" << tfg_filename << "' does not exist\n";
    exit(1);
  }
  type = ::magic_file(handle, tfg_filepath.native().c_str());
  if (type) {
    stringstream ss;
    ss << type;
    string typeName = ss.str();
    CPP_DBG_EXEC(EQCHECK, cout << "typeName = " << typeName << endl);
    if (typeName.find("ELF") != string::npos) {
      CPP_DBG_EXEC(EQCHECK, cout << "Calling harvest on '" << typeName << "' file '" << tfg_filename << "' for " << function_name.get_value() << "\n");
      stringstream ss;
      string harvest_filename = (harvest_output_file.get_value() == "") ? tfg_filename + ".harvest" : harvest_output_file.get_value();
      //string harvest_log_filename = harvest_filename + ".log";
      string harvest_log_filename = harvest_log_file.get_value();
      if (harvest_log_filename == "") {
        harvest_log_filename = harvest_filename + ".log";
      }
      //if (function_name.get_value() != "ALL") {
      //  NOT_IMPLEMENTED();
      //}
      MSG("Harvesting object code to obtain harvested assembly...");
      ss << SUPEROPT_INSTALL_DIR << "/bin/harvest " << " -functions_only -live_callee_save -allow_unsupported -no_canonicalize_imms -no_eliminate_unreachable_bbls -no_eliminate_duplicates -f " << function_name.get_value() << " -o " << harvest_filename << " -l " << harvest_log_filename << " " << tfg_filename;
      xsystem(ss.str().c_str());
      CPP_DBG_EXEC(EQCHECK, cout << "Calling eqgen on '" << harvest_filename << "'\n");
      string new_tfg_filename = tfg_filename + ".tfg";
      ss.str("");
      MSG("Converting harvested assembly to Transfer Function Graph (TFG)...");
      ss << SUPEROPT_INSTALL_DIR << "/bin/eqgen " << " -tfg_llvm " << etfg_filename << " -l " << harvest_log_filename << " -o " << new_tfg_filename << " -e " << tfg_filename << " -f " << function_name.get_value() << " " << harvest_filename;
      xsystem(ss.str().c_str());
      tfg_filename = new_tfg_filename;
    }
  }

  ifstream in_dst(tfg_filename);
  if (!in_dst.is_open()) {
    cout << __func__ << " " << __LINE__ << ": Could not open tfg_filename '" << tfg_filename << "'\n";
  }
  ASSERT(in_dst.is_open());
  string cur_function_name;

  unlink(proof_filename.c_str());

  stats::get().set_result_suffix(result_suffix.get_value());

  ASSERT(g_correl_locals_flag);
  if (disable_correl_locals.get_value()) {
    g_correl_locals_flag = false;
  }

  //map<string> function_names_for_which_proof_already_exists = eqcheck::read_existing_proof_file(existing_proof_file.get_value());

  bool eq_result = true;
  set<string> failed_functions;
  map<string, shared_ptr<corr_graph const>> fname_cg_map;

//  MSG("Computing equivalence of the two TFGs (LLVM IR and x86 assembly)...");
  while (getNextFunctionInTfgFile(in_dst, cur_function_name)) {
    ctx->clear_cache();
    if (   function_name.get_value() != "ALL"
        && function_name.get_value() != cur_function_name) {
      skip_till_next_function(in_dst);
      continue;
    }
    //if (function_names_for_which_proof_already_exists.count(cur_function_name)) {
    //  skip_till_next_function(in_dst);
    //  continue;
    //}
    if (cur_function_name == "mymemcpy" || cur_function_name == "mymemset") {
      ostringstream ss;
      ss << "Ignoring special function: " << cur_function_name << endl;
      MSG(ss.str().c_str());
      cout << ss.str();
      skip_till_next_function(in_dst);
      continue;
    }
    rodata_map_t rodata_map;
    shared_ptr<tfg> t_dst;
    shared_ptr<tfg> t_llvm;

    vector<dst_insn_t> dst_iseq_vec;
    vector<dst_ulong> dst_insn_pcs_vec;

    bool check = parse_input_eq_file(cur_function_name, etfg_filename, in_dst, &t_dst, &t_llvm, rodata_map, dst_iseq_vec, dst_insn_pcs_vec, ctx, llvm2ir.get_value());
    if (!check) {
      cout << "Invalid eq file" << endl;
      exit(1); //return 1 if does not need further exploration
    }

    auto chrono_start = chrono::system_clock::now();
    //stats::get().clear_counter("smt-queries");
    //stats::get().get_timer("query:smt")->clear();
    //stats::get().get_timer("decide_hoare_triple.ce")->clear();
    stats::get().clear();
    stats::get().set_value("dst-aloc", dst_iseq_vec.size());
    shared_ptr<corr_graph const> cur_eq_result = genEqProof(proof_filename, cur_function_name, t_llvm, t_dst, rodata_map, dst_iseq_vec, dst_insn_pcs_vec, ctx, llvm2ir.get_value(), undef_behaviour_to_be_ignored, assume_file.get_value(), local_sprel_assumes_file.get_value(), check_filename.get_value(), qt);
    unsigned num_smt_queries = stats::get().get_counter_value("smt-queries");
    auto chrono_now = chrono::system_clock::now();
    auto elapsed = chrono_now - chrono_start;
    //auto smt_queries_time_elapsed = stats::get().get_timer("query:smt")->get_elapsed();
    //cout << "PASS equivalence proof for function " << cur_function_name << ". " << chrono_duration_to_string(elapsed) << "s, " << num_smt_queries << " smtq" << endl;
    cout << (cur_eq_result ? "PASS" : "FAIL") << " equivalence check for function " << cur_function_name << ". " << chrono_duration_to_string(elapsed) << "s, " << /*num_smt_queries << "smtq" << ", " <<*/ stats::get().get_timer("query:smt")->to_string() << ", " << stats::get().get_timer("decide_hoare_triple.ce")->to_string() << ", " << stats::get().get_timer("decide_hoare_triple_helper")->to_string() << endl;
    if (!cur_eq_result) {
      eq_result = false;
      failed_functions.insert(cur_function_name);
      if (stop_on_first_failure.get_value()) {
        break;
      }
    } else {
      fname_cg_map.insert(make_pair(cur_function_name, cur_eq_result));
    }
  }

  in_dst.close();

  if (annotate_assembly_file.get_value() != "") {
    ofstream ofs(annotate_assembly_output_filename);
    function_cg_map_t function_cg_map(fname_cg_map);
    function_cg_map.remove_marker_calls_and_gen_annotated_assembly(ofs, annotate_assembly_file.get_value());
    ofs.close();
  }

  //solver_kill(); // DO NOT UNCOMMENT IF YOU WANT TO GET SOLVER STATS
  call_on_exit_function();

  if (cleanup_query_files.get_value()) {
    cout << __FILE__ << " " << __func__ << " " << __LINE__ << ": cleaning query files\n";
    clean_query_files();
    cout << __FILE__ << " " << __func__ << " " << __LINE__ << ": done cleaning query files\n";
  }
  cout << stats::get().get_timer("total-equiv")->to_string();
  if (eq_result) {
    cout << "eq-found." + result_suffix.get_value() + ":\n" << "\n-------- SMILE! :) --------\n" << endl;
    exit(0); //return 0 if does not need further exploration; 0 signifies success
  } else {
    cout << "Failing functions:";
    for (const auto &f : failed_functions) {
      cout << " " << f;
    }
    cout << "\n";
    cout << "eq-not-found." + result_suffix.get_value() + ":\n" << endl;
    exit(1); //return 1 if needs further exploration
  }
}
