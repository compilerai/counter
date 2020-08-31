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
#include "ptfg/optflags.h"
#include "support/c_utils.h"
#include "valtag/elf/elf.h"
#include "valtag/symbol.h"
#include "superopt/harvest.h"
#include "i386/insn.h"
#include "codegen/etfg_insn.h"
#include "rewrite/assembly_handling.h"

#include <iostream>
#include <string>
#include <boost/filesystem.hpp>
#include <boost/range.hpp>
#include <magic.h>

#define DEBUG_TYPE "main"

using namespace std;

static string
change_extension(string const& objfile, string const& suffix)
{
  boost::filesystem::path objpath(objfile);
  boost::filesystem::path stem = objpath.stem();
  boost::filesystem::path objdir = objpath.parent_path();
  string objdirname = objdir.native();
  string filename = stem.native() + suffix;
  string proof_file = (objdirname == "") ? filename : objdirname + "/" + filename;
  return proof_file;
}

static string
asm2obj_filename(string const& objfile)
{
  return change_extension(objfile, ".o");
}

static string
obj2proof_filename(string const& objfile)
{
  return change_extension(objfile, ".proof");
}

static string
coverage_notes_filename_to_proof_filename(string const& coverage_notes_filename)
{
  return change_extension(coverage_notes_filename, ".proof");
}

static string
obj2harvest_output_filename(string const& objfile)
{
  return change_extension(objfile, ".harvest");
}

static string
ll2bc(string const& llfile)
{
  boost::filesystem::path llpath(llfile);
  boost::filesystem::path stem = llpath.stem();
  boost::filesystem::path lldir = llpath.parent_path();
  string bcfile = lldir.native() + "/" + stem.native() + ".bc";
  stringstream ss;
  ss << SUPEROPT_INSTALL_DIR << "/bin/llvm-as " << llfile << " -o " << bcfile;
  cout << "Executing command: '" << ss.str() << "'\n";
  xsystem(ss.str().c_str());
  return bcfile;
}

static string
bc_opt(string const& bcfile, optflags_t const& optflags)
{
  boost::filesystem::path bcpath(bcfile);
  boost::filesystem::path stem = bcpath.stem();
  boost::filesystem::path bcdir = bcpath.parent_path();
  //cout << __func__ << " " << __LINE__ << ": bcfile = " << bcfile << ", bcdir = " << bcdir.native() << endl;
  string bcfile_opt = bcdir.native() + "/" + stem.native() + ".opt.bc";
  stringstream ss;
  ss << SUPEROPT_INSTALL_DIR << "/bin/opt " << optflags.get_cmdflags() << " -strip-debug " << bcfile << " -o " << bcfile_opt;
  cout << "Executing command: '" << ss.str() << "'\n";
  xsystem(ss.str().c_str());
  return bcfile_opt;
}

static pair<asmfile_t, objfile_t>
llc(string const& bcfile, optflags_t const& optflags)
{
  boost::filesystem::path bcpath(bcfile);
  boost::filesystem::path stem = bcpath.stem();
  boost::filesystem::path bcdir = bcpath.parent_path();
  asmfile_t asmfile = bcdir.native() + "/" + stem.native() + ".s";
  objfile_t objfile = bcdir.native() + "/" + stem.native() + ".o";
  stringstream ss;
  ss << SUPEROPT_INSTALL_DIR << "/bin/llc " << optflags.get_cmdflags() << " " << bcfile << " -filetype=asm -o " << asmfile;
  cout << "Executing command: '" << ss.str() << "'\n";
  xsystem(ss.str().c_str());
  ss.str("");
  ss << SUPEROPT_INSTALL_DIR << "/bin/llc " << optflags.get_cmdflags() << " " << bcfile << " -filetype=obj -o " << objfile;
  cout << "Executing command: '" << ss.str() << "'\n";
  xsystem(ss.str().c_str());
  return make_pair(asmfile, objfile);
}

static bool
eq(string const& bcfile, string const& asmfile, string const& objfile, string const& proof_file, string const& harvest_file, string const& harvest_log, string& asmfile_annotated, string const& debug_opts)
{
  boost::filesystem::path asmpath(asmfile);
  boost::filesystem::path asmdir = asmpath.parent_path();
  boost::filesystem::path stem = asmpath.stem();
  asmfile_annotated = asmdir.native() + "/" + stem.native() + ".annot.s";
  stringstream ss;
  ss << SUPEROPT_INSTALL_DIR << "/bin/eq " << bcfile << " " << objfile << " --stop-on-first-failure --proof " << proof_file << " --harvest " << harvest_file << " --harvest_log " << harvest_log << " --annotate-assembly " << asmfile << " -annotate-assembly-output " << asmfile_annotated;
  if (debug_opts.size())
    ss << " --debug=" << debug_opts;
  cout << __func__ << " " << __LINE__ << ": cmd =\n" << ss.str() << endl;
  int ret = system(ss.str().c_str());
  return WEXITSTATUS(ret) == 0;
}

static void
bcfiles_link(vector<bcfile_t> const& bcfiles, string const& outname)
{
  stringstream ss;
  ss << SUPEROPT_INSTALL_DIR << "/bin/llvm-link";
  for (auto const& bcfile : bcfiles) {
    ss << " " << bcfile;
  }
  ss << " -o " << outname;
  cout << "Executing command: '" << ss.str() << "'\n";
  xsystem(ss.str().c_str());
}

static void
objfiles_link(vector<objfile_t> const& objfiles, string const& outname)
{
  stringstream ss;
  ss << SUPEROPT_INSTALL_DIR << "/bin/qcc-ld -relocatable";
  //ss << SUPEROPT_DIR << "/build/third_party/binutils-2.21-install/bin/ld -relocatable";
  for (auto const& objfile : objfiles) {
    ss << " " << objfile;
  }
  ss << " -o " << outname;
  cout << "Executing command: '" << ss.str() << "'\n";
  xsystem(ss.str().c_str());
}

static void
files_concat(vector<proof_file_t> const& proof_files, string const& outname)
{
  stringstream ss;
  ss << "/bin/cat";
  for (auto const& proof_file : proof_files) {
    ss << " " << proof_file;
  }
  ss << " > " << outname;
  cout << "Executing command: '" << ss.str() << "'\n";
  xsystem(ss.str().c_str());
}

static bool
instrumentMarkerCall(string & bcfile, string const& function_name)
{
  string output_filename = increment_marker_number(bcfile);
  //cout << __func__ << " " << __LINE__ << ": output_filename = '" << output_filename << "'\n";
  stringstream ss;
  ss << SUPEROPT_INSTALL_DIR << "/bin/opt -load " << SUPEROPT_INSTALL_DIR << "/lib/LLVMSuperopt.so " << bcfile << " -loops -instrumentMarkerCall -choose-function " << function_name << " -o " << output_filename;
  cout << "Executing command: '" << ss.str() << "'\n";
  cout.flush();
  xsystem(ss.str().c_str());
  if (files_differ(bcfile, output_filename)) {
    bcfile = output_filename;
    return true;
  } else {
    cout << "files '" << bcfile << "' and '" << output_filename << "' are identical, indicating that instrumentation of a marker call failed. returning false.\n";
    return false;
  }
}

static tuple<bcfile_t, asmfile_t, objfile_t, proof_file_t, harvest_file_t, harvest_log_t, asmfile_t>
qcc_build(string bcfile, optflags_t const& optflags, string const& debug_opts)
{
  size_t num_tries = 0;
  //cout << "Generating code for llfile " << llfile << "'\n";
  //string bcfile = ll2bc(llfile);
  cout << "Generating code for bcfile " << bcfile << "'\n";
  while (num_tries <= QCC_BUILD_MAX_TRIES) {
    MSG("Generating optimized LLVM bitcode from unoptimized LLVM bitcode...");
    string bcfile_opt = bc_opt(bcfile, optflags);
    cout << "optimized bcfile " << bcfile << " into bcfile_opt " << bcfile_opt << "'\n";
    MSG("Generating assembly code from optimized LLVM bitcode using llc...");
    pair<asmfile_t, objfile_t> pr = llc(bcfile_opt, optflags);
    asmfile_t asmfile = pr.first;
    objfile_t objfile = pr.second;
    string proof_file = obj2proof_filename(objfile);
    string harvest_output_file = obj2harvest_output_filename(objfile);
    string harvest_log_file = harvest_output_file + ".log";
    asmfile_t asmfile_annotated;
    MSG("Validating generated assembly code against unoptimized LLVM bitcode...");
    if (eq(bcfile, asmfile, objfile, proof_file, harvest_output_file, harvest_log_file, asmfile_annotated, debug_opts)) {
      cout << "eq succeeded for bcfile " << bcfile << " and objfile " << objfile << "' into proof file '" << proof_file << "'\n";
      MSG("Succeeded in validating generated assembly code against unoptimized LLVM bitcode...");
      return make_tuple(bcfile, asmfile, objfile, proof_file, harvest_output_file, harvest_log_file, asmfile_annotated);
    }
    string failed_function_name = eqcheck::proof_file_identify_failed_function_name(proof_file);
    cout << "eq failed for function '" << failed_function_name << "' across bcfile " << bcfile << " and objfile " << objfile << "'\n";
    MSG("Validation failed; instrumenting marker call...");
    if (!instrumentMarkerCall(bcfile, failed_function_name)) {
      cout << "could not instrument marker call in  '" << bcfile << "'\n";
      MSG("Could not instrument marker call. Validated compilation failed.");
      break;
    }
    MSG("Successfully instrumented marker call in unoptimized LLVM IR bitcode.");
    cout << "instrumented marker call to create new bcfile: '" << bcfile << "'\n";
  }
  cout << __func__ << " " << __LINE__ << ": Could not validate the compilation of " << bcfile << ". exiting with code 1\n";
  exit(1);
}

static void
mem2reg(string const& bcfile, string const& output)
{
  cout << "Performing mem2reg on '" << bcfile << "' to produce '" << output << "'\n";
  stringstream ss;
  ss << SUPEROPT_INSTALL_DIR << "/bin/opt -mem2reg -strip-debug " << bcfile << " -o " << output;
  cout << "Executing command: '" << ss.str() << "'\n";
  xsystem(ss.str().c_str());
}

int main(int argc, char **argv)
{
  autostop_timer func_timer(__func__);

  src_init();
  dst_init();

  cl::cl cmd("Equivalence checker: checks equivalence of given .eq file");
  // command line args processing
  cl::arg<string> bc_file(cl::implicit_prefix, "", "path to .bc file");
  cl::arg<string> output_file(cl::explicit_prefix, "o", "out.qcc", "output filename");

  // eq args
  cl::arg<string> function_name(cl::explicit_prefix, "f", "ALL", "function name to be compared for equivalence");
  cl::arg<string> record_filename(cl::explicit_prefix, "record", "", "generate record log of SMT queries to given filename");
  cl::arg<string> replay_filename(cl::explicit_prefix, "replay", "", "replay log of SMT queries from given filename");
  cl::arg<string> check_filename(cl::explicit_prefix, "check", "", "Check proof (do not generate proof) stored in the argument (filename)");
  cl::arg<bool> read_only(cl::explicit_flag, "read_only", false, "Read and print stats for CG.  Do not check anything.  'check' filename must be supplied for this.");
  cl::arg<string> solver_stats_filename(cl::explicit_prefix, "solver_stats", "", "save smt solver statistics to given filename");
  cl::arg<unsigned> page_size(cl::explicit_prefix, "page_size", 4096, "Check proof (do not generate proof) stored in the argument (filename)");
  cl::arg<unsigned> unroll_factor(cl::explicit_prefix, "unroll-factor", 1, "Loop unrolling factor");
  cl::arg<unsigned> dst_unroll_factor(cl::explicit_prefix, "dst-unroll-factor", 1, "Dst graph loop unrolling factor");
  cl::arg<bool> cleanup_query_files(cl::explicit_flag, "cleanup-query-files", false, "Cleanup all query files like is_expr_equal_using_lhs_set, etc., on pass result");
  cl::arg<unsigned> smt_query_timeout(cl::explicit_prefix, "smt-query-timeout", 3600, "Timeout per query (s)");
  cl::arg<unsigned> sage_query_timeout(cl::explicit_prefix, "sage-query-timeout", 600, "Timeout per query (s)");
  cl::arg<unsigned> global_timeout(cl::explicit_prefix, "global-timeout", 36000, "Total timeout (s)");
  cl::arg<unsigned long> max_address_space_size(cl::explicit_prefix, "max-address-space-size", (unsigned long)-1, "Max address space size");
  cl::arg<string> proof_file(cl::explicit_prefix, "proof", "", "File to write proof");
  //cl::arg<bool> force_acyclic(cl::explicit_flag, "force_acyclic", false, "Collapse the TFGs so that acyclic TFGs are reduced to a single edge");
  //cl::arg<bool> test_acyclic(cl::explicit_flag, "test_acyclic", false, "Test equivalence of acyclic TFGs against collapsed TFGs");
  cl::arg<string> assume_file(cl::explicit_prefix, "assume", "", "File to read assume clauses from (for debugging)");
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
  cl::arg<bool> disable_simplify_cache(cl::explicit_flag, "disable_simplify_cache", false, "Disable context simplify cache");
  cl::arg<bool> use_only_live_src_vars(cl::explicit_flag, "use_only_live_src_vars", false, "Select whether to only use live src locs or all.  Default: false (all)");
  cl::arg<bool> infer_src_tfg_invariants(cl::explicit_flag, "infer_src_tfg_invariants", false, "Select whether to infer src tfg invariants or not.  Default: false (Do not infer)");
  cl::arg<bool> use_sage(cl::explicit_flag, "use_sage", false, "Select whether to use sage for solving BV equations or C++ implementation.  Default: false (Use C++ implementation)");
  //cl::arg<unsigned> sage_dedup_threshold(cl::explicit_prefix, "sage_dedup_threshold", 20, "Set bv_solve_sage dedup column threshold.  Default: 20");
  cl::arg<unsigned> warm_start_param(cl::explicit_prefix, "warm_start_param", 2, "The number of CE to start invariant inference  Default: 2");
  cl::arg<int> solver_config(cl::explicit_prefix, "solver_config", 0, "Which solvers to use; 0- both, 1- yices only 2- z3 only Default: 0");
  cl::arg<int> max_lookahead(cl::explicit_prefix, "max_lookahead", 0, "Max lookahead value.  Valid range: [0, GUESSING_MAX_LOOKAHEAD)");
  //cl::arg<bool> disable_heuristic_pruning(cl::explicit_flag, "disable_hpruning", false, "Disable heuristic pruning cache");
  cl::arg<bool> disable_pernode_CE_cache(cl::explicit_flag, "disable_pernode_CE_cache", false, "Disable per node CE caching for invariant inference");
  cl::arg<bool> enable_local_sprel_guesses(cl::explicit_flag, "enable_local_sprel_guesses", false, "Enable local sprel inference");
  cl::arg<bool> add_mod_exprs(cl::explicit_flag, "add_mod_exprs", false, "Add mod expressions required for checking vectorization");
  cl::arg<bool> llvm2ir(cl::explicit_flag, "llvm2ir", false, "Check one LLVM program against another");
  cl::arg<bool> disable_points_propagation(cl::explicit_flag, "disable_points_propagation", false, "disable propagation of points across edges");
  cl::arg<bool> disable_inequality_inference(cl::explicit_flag, "disable_inequality_inference", false, "Disable inequality inference");
  cl::arg<string> debug_opts(cl::explicit_prefix, "debug", "", "Enable dynamic debugging for debug-class(es).  Expects comma-separated list of debug-classes with optional level e.g. --debug=correlate=2,smt_query=1");

  cl::arg<bool> clang_asm_output(cl::explicit_flag, "S", false, "generate .s assembly output");
  cmd.add_arg(&clang_asm_output);
  cl::arg<bool> clang_cc1(cl::explicit_flag, "cc1", false, "clang cc1");
  cmd.add_arg(&clang_cc1);
  cl::arg<string> clang_triple(cl::explicit_prefix, "triple", "i386-unknown-linux", "clang cc1");
  cmd.add_arg(&clang_triple);
  cl::arg<bool> clang_emit_obj(cl::explicit_flag, "emit-obj", false, "clang flag");
  cmd.add_arg(&clang_emit_obj);
  cl::arg<bool> clang_relax_all(cl::explicit_flag, "mrelax-all", false, "clang flag");
  cmd.add_arg(&clang_relax_all);
  cl::arg<bool> clang_disable_free(cl::explicit_flag, "disable-free", false, "clang flag");
  cmd.add_arg(&clang_disable_free);
  cl::arg<string> clang_main_filename(cl::explicit_prefix, "main-file-name", "", "clang flag");
  cmd.add_arg(&clang_main_filename);
  cl::arg<string> clang_relocation_model(cl::explicit_prefix, "mrelocation-model", "static", "clang flag");
  cmd.add_arg(&clang_relocation_model);
  cl::arg<string> clang_thread_model(cl::explicit_prefix, "mthread-model", "posix", "clang flag");
  cmd.add_arg(&clang_thread_model);
  cl::arg<bool> clang_fp_elim(cl::explicit_flag, "mdisable-fp-elim", false, "clang flag");
  cmd.add_arg(&clang_fp_elim);
  cl::arg<bool> clang_math_errno(cl::explicit_flag, "fmath-errno", false, "clang flag");
  cmd.add_arg(&clang_math_errno);
  cl::arg<bool> clang_masm_verbose(cl::explicit_flag, "masm-verbose", false, "clang flag");
  cmd.add_arg(&clang_masm_verbose);
  cl::arg<bool> clang_constructor_aliases(cl::explicit_flag, "mconstructor-aliases", false, "clang flag");
  cmd.add_arg(&clang_constructor_aliases);
  cl::arg<bool> clang_unwind_tables(cl::explicit_flag, "munwind-tables", false, "clang flag");
  cmd.add_arg(&clang_unwind_tables);
  cl::arg<bool> clang_fuse_init_array(cl::explicit_flag, "fuse-init-array", false, "clang flag");
  cmd.add_arg(&clang_fuse_init_array);
  cl::arg<string> clang_target_cpu(cl::explicit_prefix, "target-cpu", "pentium4", "clang flag");
  cmd.add_arg(&clang_target_cpu);
  cl::arg<bool> clang_dwarf_column_info(cl::explicit_flag, "dwarf-column-info", false, "clang flag");
  cmd.add_arg(&clang_dwarf_column_info);
  cl::arg<string> clang_debugger_tuning(cl::explicit_prefix, "debugger-tuning", "gdb", "clang flag");
  cmd.add_arg(&clang_debugger_tuning);
  cl::arg<string> clang_resource_dir(cl::explicit_prefix, "resource-dir", "", "clang flag");
  cmd.add_arg(&clang_resource_dir);
  cl::arg<string> clang_debug_compilation_dir(cl::explicit_prefix, "fdebug-compilation-dir", "", "clang flag");
  cmd.add_arg(&clang_debug_compilation_dir);
  cl::arg<int> clang_error_limit(cl::explicit_prefix, "ferror-limit", 19, "clang flag");
  cmd.add_arg(&clang_error_limit);
  cl::arg<int> clang_message_length(cl::explicit_prefix, "fmessage-length", 80, "clang flag");
  cmd.add_arg(&clang_message_length);
  cl::arg<string> clang_objc_runtime(cl::explicit_prefix, "fobjc-runtime", "gcc", "clang flag");
  cmd.add_arg(&clang_objc_runtime);
  cl::arg<bool> clang_diagnostics_show_option(cl::explicit_flag, "fdiagnostics-show-option", false, "clang flag");
  cmd.add_arg(&clang_diagnostics_show_option);
  cl::arg<bool> clang_color_diagnostics(cl::explicit_flag, "fcolor-diagnostics", false, "clang flag");
  cmd.add_arg(&clang_color_diagnostics);
  cl::arg<string> clang_x(cl::explicit_prefix, "x", "ir", "clang flag");
  cmd.add_arg(&clang_x);
  cl::arg<bool> clang_addrsig(cl::explicit_flag, "faddrsig", false, "clang flag");
  cmd.add_arg(&clang_addrsig);
  cl::arg<bool> clang_disable_O0_optnone(cl::explicit_flag, "disable-O0-optnone", false, "clang flag");
  cmd.add_arg(&clang_disable_O0_optnone);
  cl::arg<int> clang_dwarf_version(cl::explicit_prefix, "dwarf-version", 4, "clang dwarf-version prefix");
  cmd.add_arg(&clang_dwarf_version);
  cl::arg<string> clang_std(cl::explicit_prefix, "std", "c11", "clang C revision to be used");
  cmd.add_arg(&clang_std);
  cl::arg<bool> clang_fwrapv(cl::explicit_flag, "fwrapv", false, "clang flag");
  cmd.add_arg(&clang_fwrapv);
  cl::arg<bool> clang_no_unroll_loops(cl::explicit_flag, "fno-unroll-loops", false, "clang flag");
  cmd.add_arg(&clang_no_unroll_loops);
  cl::arg<bool> clang_no_builtin(cl::explicit_flag, "fno-builtin", false, "clang flag");
  cmd.add_arg(&clang_no_builtin);
  cl::arg<bool> clang_no_inline(cl::explicit_flag, "fno-inline", false, "clang flag");
  cmd.add_arg(&clang_no_inline);
  cl::arg<bool> clang_no_inline_functions(cl::explicit_flag, "fno-inline-functions", false, "clang flag");
  cmd.add_arg(&clang_no_inline_functions);

  //clang optimization flags
  cl::arg<bool> clang_vectorize_slp(cl::explicit_flag, "vectorize-slp", false, "clang flag");
  cmd.add_arg(&clang_vectorize_slp);
  cl::arg<bool> clang_omit_leaf_frame_pointer(cl::explicit_flag, "momit-leaf-frame-pointer", false, "clang flag");
  cmd.add_arg(&clang_omit_leaf_frame_pointer);
  cl::arg<bool> clang_O(cl::explicit_flag, "O", false, "clang O optimization flag");
  cmd.add_arg(&clang_O);
  cl::arg<bool> clang_O0(cl::explicit_flag, "O0", false, "clang O0 optimization flag");
  cmd.add_arg(&clang_O0);
  cl::arg<bool> clang_O1(cl::explicit_flag, "O1", false, "clang O1 optimization flag");
  cmd.add_arg(&clang_O1);
  cl::arg<bool> clang_O2(cl::explicit_flag, "O2", false, "clang O2 optimization flag");
  cmd.add_arg(&clang_O2);
  cl::arg<bool> clang_O3(cl::explicit_flag, "O3", false, "clang O3 optimization flag");
  cmd.add_arg(&clang_O3);
  cl::arg<bool> clang_no_zero_initialized_in_bss(cl::explicit_flag, "mno-zero-initialized-in-bss", false, "clang -mno-zero-initialized-in-bss flag");
  cmd.add_arg(&clang_no_zero_initialized_in_bss);
  cl::arg<bool> clang_relaxed_aliasing(cl::explicit_flag, "relaxed-aliasing", false, "clang -relaxed-aliasing flag");
  cmd.add_arg(&clang_relaxed_aliasing);
  cl::arg<bool> clang_disable_tail_calls(cl::explicit_flag, "mdisable-tail-calls", false, "clang -mdisable-tail-calls flag");
  cmd.add_arg(&clang_disable_tail_calls);
  cl::arg<bool> clang_freestanding(cl::explicit_flag, "ffreestanding", false, "clang -ffreestanding flag");
  cmd.add_arg(&clang_freestanding);
  cl::arg<string> clang_target_feature(cl::explicit_prefix, "target-feature", "", "clang -target-feature prefix");
  cmd.add_arg(&clang_target_feature); //XXX: TODO: need to use target-feature; also add support to cl if multiple such specifications are possible in clang
  cl::arg<string> clang_debug_info_kind(cl::explicit_prefix, "debug-info-kind", "", "clang -debug-info-kind prefix");
  cmd.add_arg(&clang_debug_info_kind);
  cl::arg<string> clang_coverage_notes_file(cl::explicit_prefix, "coverage-notes-file", "", "clang -coverage-notes-file");
  cmd.add_arg(&clang_coverage_notes_file);
  cl::arg<bool> clean_qcc_dir(cl::explicit_flag, "clean-qcc-dir", false, "clean (delete all files in) qcc directory (/tmp/qcc-files) before starting");
  cmd.add_arg(&clean_qcc_dir);

  cmd.add_arg(&function_name);
  cmd.add_arg(&record_filename);
  cmd.add_arg(&replay_filename);
  cmd.add_arg(&check_filename);
  cmd.add_arg(&read_only);
  cmd.add_arg(&solver_stats_filename);
  cmd.add_arg(&page_size);
	cmd.add_arg(&bc_file);
  cmd.add_arg(&output_file);
  cmd.add_arg(&result_suffix);
  cmd.add_arg(&unroll_factor);
  cmd.add_arg(&dst_unroll_factor);
  cmd.add_arg(&cleanup_query_files);
  cmd.add_arg(&smt_query_timeout);
  cmd.add_arg(&sage_query_timeout);
  cmd.add_arg(&global_timeout);
  cmd.add_arg(&max_address_space_size);
  cmd.add_arg(&proof_file);
  //cmd.add_arg(&test_acyclic);
  //cmd.add_arg(&force_acyclic);
  cmd.add_arg(&assume_file);
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
  cmd.add_arg(&disable_simplify_cache);
  cmd.add_arg(&use_only_live_src_vars);
  cmd.add_arg(&infer_src_tfg_invariants);
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
  cmd.add_arg(&debug_opts);

  CPP_DBG_EXEC(ARGV_PRINT,
      for (int i = 0; i < argc; i++) {
        cout << " " << argv[i];
      }
      cout << endl;
  );
  cmd.parse(argc, argv);

  eqspace::init_dyn_debug_from_string(debug_opts.get_value());

  string bc_filename = bc_file.get_value();

  optflags_t optflags = optflags_t::construct_optflags(clang_vectorize_slp.get_value(), clang_omit_leaf_frame_pointer.get_value(), clang_O.get_value(), clang_O0.get_value(), clang_O1.get_value(), clang_O2.get_value(), clang_O3.get_value());

  string qcc_dir = "/tmp/qcc-files";
  if (clean_qcc_dir.get_value()) {
    boost::filesystem::remove_all(qcc_dir);
  }
  boost::filesystem::path qcc_dirpath(qcc_dir.c_str());
  if (!boost::filesystem::exists(qcc_dirpath)) {
    if (boost::filesystem::create_directory(qcc_dirpath)) {
      cout << "Directory Created: " << qcc_dir << endl;
    }
  }
  boost::filesystem::path bc_path(bc_filename);
  boost::filesystem::path bc_filename_name = bc_path.filename();
  string new_bc_filename = qcc_dir + "/" + bc_filename_name.native();
  boost::filesystem::remove(new_bc_filename);
  //create_symlink(boost::filesystem::canonical(bc_path), new_bc_filename);
  MSG("Preparing LLVM bitcode file...");
  mem2reg(boost::filesystem::canonical(bc_path).native(), new_bc_filename);

  //cout << "Splitting functions of '" << new_bc_filename << "'\n";
  //stringstream ss;
  //ss << SUPEROPT_INSTALL_DIR << "/bin/opt -load " << SUPEROPT_INSTALL_DIR << "/lib/LLVMSuperopt.so " << new_bc_filename << " -splitModule -disable-output";
  //cout << "Executing command: '" << ss.str() << "'\n";
  //xsystem(ss.str().c_str());
  //cout << "done Splitting functions of '" << new_bc_filename << "'\n";

  //list<pair<bcfile_t, objfile_t>> result;
  vector<bcfile_t> bcfiles;
  vector<asmfile_t> asmfiles;
  vector<objfile_t> objfiles;
  vector<proof_file_t> proof_files;
  vector<harvest_file_t> harvest_files;
  vector<harvest_log_t> harvest_logs;
  vector<asmfile_t> asmfiles_annotated;
  for (auto& entry : boost::make_iterator_range(boost::filesystem::directory_iterator(qcc_dir), {})) {
    if (entry.path() != new_bc_filename) {
      continue;
    }
    tuple<bcfile_t, asmfile_t, objfile_t, proof_file_t, harvest_file_t, harvest_log_t, asmfile_t> pr = qcc_build(entry.path().native(), optflags, debug_opts.get_value());
    //result.push_back(pr);
    ASSERT(bcfiles.size() == 00);
    bcfiles.push_back(get<0>(pr));
    asmfiles.push_back(get<1>(pr));
    objfiles.push_back(get<2>(pr));
    proof_files.push_back(get<3>(pr));
    harvest_files.push_back(get<4>(pr));
    harvest_logs.push_back(get<5>(pr));
    asmfiles_annotated.push_back(get<6>(pr));
  }

  boost::filesystem::path bc_filename_stem = bc_path.stem();
  bcfile_t bcresult = qcc_dir + "/" + bc_filename_stem.native() + ".out.bc";
  //objfile_t objresult = qcc_dir + "/" + bc_filename_stem.native() + ".out.i386";
  string asmresult, objresult;
  if (clang_asm_output.get_value()) {
    asmresult = output_file.get_value();
    objresult = asm2obj_filename(asmresult);
  } else {
    objresult = output_file.get_value();
    asmresult = change_extension(objresult, ".s");
  }
  //proof_file_t proof_result = obj2proof_filename(objresult);
  proof_file_t proof_result = coverage_notes_filename_to_proof_filename(clang_coverage_notes_file.get_value());
  harvest_file_t harvest_result = obj2harvest_output_filename(objresult);
  harvest_log_t harvest_log = harvest_result + ".log";

  bcfiles_link(bcfiles, bcresult);
  objfiles_link(objfiles, objresult);
  files_concat(proof_files, proof_result);
  files_concat(harvest_files, harvest_result);
  gen_unified_harvest_log(harvest_logs, harvest_log);
  //gen_assembly_from_harvest_output(asmresult, harvest_result, harvest_log);
  assembly_file_t::gen_unified_assembly_from_assembly_files(asmresult, asmfiles_annotated);

  ostringstream ss;
  ss << "Validated compilation successful: " << bcresult << " -> " << objresult << " [proof " << proof_result << ", harvest " << harvest_result << ", assembly " << asmresult << "]\n";
  //cout << "output_file " << output_file.get_value() << endl;
  cout << ss.str();
  MSG("Congratulations! Validated compilation successful\n");
  return 0;
}
