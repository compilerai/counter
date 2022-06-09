//===-- llvm-dis.cpp - The low-level LLVM disassembler --------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
//
//===----------------------------------------------------------------------===//

#include "llvm/InitializePasses.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/Bitcode/BitcodeReader.h"
#include "llvm/IR/AssemblyAnnotationWriter.h"
#include "llvm/IR/DebugInfo.h"
#include "llvm/IR/DiagnosticInfo.h"
#include "llvm/IR/DiagnosticPrinter.h"
#include "llvm/IR/IntrinsicInst.h"
#include "llvm/IRReader/IRReader.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Type.h"
#include "llvm/Support/CommandLine.h"
//#include "llvm/Support/DataStream.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/FormattedStream.h"
#include "llvm/Support/ManagedStatic.h"
#include "llvm/Support/MemoryBuffer.h"
#include "llvm/Support/PrettyStackTrace.h"
#include "llvm/Support/Signals.h"
#include "llvm/Support/ToolOutputFile.h"
#include "llvm/IR/IRPrintingPasses.h"
#include <system_error>
using namespace llvm;

#include <iostream>
#include <fstream>

#include "support/timers.h"
#include "support/dyn_debug.h"
#include "support/globals.h"

#include "expr/consts_struct.h"
#include "expr/expr.h"

#include "tfg/tfg_llvm.h"

#include "eq/eqcheck.h"
#include "ptfg/llptfg.h"
#include "ptfg/function_signature.h"

#include "sym_exec_llvm.h"

using namespace eqspace;

#include <signal.h>

static cl::opt<std::string>
InputFilename1(cl::Positional, cl::desc("<input bitcode file1>"), cl::init("-"));

static cl::opt<std::string>
InputFilename2(cl::Positional, cl::desc("<input bitcode file2>"), cl::init("-"));

static cl::opt<std::string>
FunNames("f", cl::desc("<funname>"), cl::init(""));
static cl::opt<std::string>
OutputFilename("o", cl::desc("<output>"), cl::init("out.etfg"));

static cl::opt<std::string>
src_etfg_filename("src-etfg", cl::desc("source ETFG filename (so that the symbol ids and local ids can be consistent between the generated ETFG and the src-etfg"), cl::init(""));

static cl::opt<std::string>
xml_output_file("xml-output", cl::desc("<xml output>"), cl::init(""));

static cl::opt<bool>
DisableModelingOfUninitVarUB("u", cl::desc("<disable-modeling-of-uninit-var-ub>"), cl::init(false));

static cl::opt<bool>
DryRun("dry-run", cl::desc("<dry-run. only print the function names and their sizes>"), cl::init(false));

static cl::opt<std::string>
DynDebug("dyn-debug", cl::desc("<dyn_debug.  enable dynamic debugging for debug-class(es).  Expects comma-separated list of debug-classes with optional level e.g. -dyn_debug=compute_liveness,sprels,alias_analysis=2"), cl::init(""));

static cl::opt<bool>
llvmSemantics("llvm-semantics", cl::desc("<llvm-semantics.  Model poison and undef values (LLVM semantics) instead of just plain UB."), cl::init(false));

static cl::opt<int>
call_context_depth("call-context-depth", cl::desc("<call-context-depth.  The call context depth to use for pointsto-analysis."), cl::init(0));

static cl::opt<std::string>
XmlOutputFormat("xml-output-format", cl::desc("<xml-output-format.  Format to use during xml printing.  [html|text-color|text-nocolor]"), cl::init("text-color"));

static cl::opt<bool>
NoGenScev("no-gen-scev", cl::desc("<no-gen-scev. don't generate potential scev relationships to be used for invariant inferences>"), cl::init(false));

static cl::opt<bool>
Progress("progress", cl::desc("<progress. keep printing progress involving time/mem stats>"), cl::init(false));

//static cl::opt<bool>
//NoCollapse("no-collapse", cl::desc("<no-collapse. Do not collapse basic blocks into single edges>"), cl::init(false));

//static void diagnosticHandler(const DiagnosticInfo &DI, void *Context) {
//  assert(DI.getSeverity() == DS_Error && "Only expecting errors");
//
//  raw_ostream &OS = errs();
//  OS << (char *)Context << ": ";
//  DiagnosticPrinterRawOStream DP(OS);
//  DI.print(DP);
//  OS << '\n';
//  exit(1);
//}

static std::unique_ptr<Module> readModule(LLVMContext &Context,
                                          StringRef Name) {
  SMDiagnostic Diag;
  std::unique_ptr<Module> M = parseIRFile(Name, Diag, Context);
  if (!M)
    Diag.print("llvm2tfg", errs());
  return M;
}

/*
static void
populate_return_regs(tfg *t)
{
  list<pc> exit_pcs;
  map<pc, vector<expr_ref>> ret_exprs;

  t->get_exit_pcs(exit_pcs);
  //cout << __func__ << " " << __LINE__ << ": exit_pcs.size() = " << exit_pcs.size() << endl;
  //assert(exit_pcs.size() == 1);
  for (pc const &exit_pc : exit_pcs) {
    ret_exprs[exit_pc] = vector<expr_ref>();
    ret_exprs.at(exit_pc).push_back(start_state.get_expr(m_mem_reg, start_state));
    ret_exprs.at(exit_pc).push_back(start_state.get_expr(m_io_reg, start_state));
    if (m_ret_reg.size() > 0) {
      ret_exprs.at(exit_pc).push_back(start_state.get_expr(m_ret_reg, start_state));
    }
  }

  //assert(exit_pcs.size() == 1);
  t->set_return_regs(ret_exprs);
}
*/

extern int g_helper_pid;

int
main(int argc, char **argv)
{
  autostop_timer func_timer(string(__func__));
  // Print a stack trace if we signal out.
  sys::PrintStackTraceOnErrorSignal(argv[0]);
  PrettyStackTraceProgram X(argc, argv);

  //LLVMContext &Context = getGlobalContext();
  LLVMContext Context;
  llvm_shutdown_obj Y;  // Call llvm_shutdown() on exit.

  //Context.setDiagnosticHandler(diagnosticHandler, argv[0]);

  cl::ParseCommandLineOptions(argc, argv, "llvm2tfg: file1.bc -> out.etfg\n");

  init_dyn_debug_from_string(DynDebug);
  CPP_DBG_EXEC(DYN_DEBUG, print_debug_class_levels());

  context::xml_output_format_t xml_output_format = context::xml_output_format_from_string(XmlOutputFormat);

  DYN_DEBUG(llvm2tfg, errs() << "doing functions:" << FunNames << "\n");
  DYN_DEBUG(llvm2tfg, errs() << "output filename:" << OutputFilename << "\n");

  if (xml_output_file != "") {
    g_xml_output_stream.open(xml_output_file, ios_base::app | ios_base::ate);
    ASSERT(g_xml_output_stream.is_open());
  }

  PassRegistry &Registry = *PassRegistry::getPassRegistry();
  initializeCore(Registry);
  //initializeCoroutines(Registry);
  //initializeScalarOpts(Registry);
  //initializeObjCARCOpts(Registry);
  //initializeVectorization(Registry);
  //initializeIPO(Registry);
  initializeAnalysis(Registry);

  set<string> FunNamesVec;
  size_t curpos = 0, nextpos;
  while ((nextpos = FunNames.find(',', curpos)) != string::npos) {
    string f = FunNames.substr(curpos, nextpos - curpos);
    FunNamesVec.insert(f);
    curpos = nextpos + 1;
  }
  string f = FunNames.substr(curpos);
  if (f.length() > 0 && f != "ALL") {
    FunNamesVec.insert(f);
  }
  DYN_DEBUG(llvm2tfg,
    errs() << "printing FunNamesVec:\n";
    for (auto ff : FunNamesVec) {
      errs() << ff << "\n";
    }
  );
  std::unique_ptr<Module> M1 = readModule(Context, InputFilename1);
  //std::unique_ptr<Module> M2 = readModule(Context, InputFilename2);

  if (!M1) {
    errs() << "could not open input filename:" << InputFilename1 << "\n";
  }

  assert(M1 /*&& M2*/);
  DYN_DEBUG(llvm2tfg, errs() << "InputFilename = " << InputFilename1 << "\n");
  //errs() << InputFilename2 << "\n";

  /*for(const pair<string, unsigned>& fun_name : fun_names)
  {
    errs() << fun_name.first << " : " << fun_name.second << "\n";
  }*/

  //context *ctx = new context(context::config(600, 600/*, true, true, true*/));
  g_ctx_init(false);
  context *ctx = g_ctx;
  DataLayout const& dl = M1->getDataLayout();
  unsigned pointer_size = dl.getPointerSize();
  //cout << __func__ << " " << __LINE__ << ": pointer_size = " << pointer_size << endl;
  if (pointer_size == QWORD_LEN/BYTE_LEN) {
    ctx->parse_consts_db(SUPEROPTDBS_DIR "/../etfg_x64/consts_db");
  } else if (pointer_size == DWORD_LEN/BYTE_LEN) {
    ctx->parse_consts_db(SUPEROPTDBS_DIR "/../etfg_i386/consts_db");
  } else {
    NOT_REACHED();
  }

  ofstream outputStream;
  outputStream.open(OutputFilename, ios_base::out | ios_base::trunc);

  if (DryRun) {
    for (const Function& f : *M1) {
      if (sym_exec_common::get_num_insn(f) == 0) {
        continue;
      }
      string fname = f.getName().str();
      outputStream << fname << " : " << sym_exec_common::get_num_insn(f) << "\n";
    }
    return 0;
  }

  shared_ptr<llptfg_t const> src_llptfg;
  if (src_etfg_filename != "") {
    ifstream in_src(src_etfg_filename);
    if (!in_src.is_open()) {
      cout << __func__ << " " << __LINE__ << ": parsing failed" << endl;
      NOT_REACHED();
    }
    src_llptfg = make_shared<llptfg_t const>(in_src, ctx);
  }

  if (Progress) {
    progress_flag = 1;
  }

  dshared_ptr<ftmap_t> function_tfg_map = sym_exec_llvm::get_function_tfg_map(M1.get(), FunNamesVec, ctx, src_llptfg, !NoGenScev, llvmSemantics, nullptr, xml_output_format);
  function_tfg_map->ftmap_run_pointsto_analysis(false, dshared_ptr<tfg_llvm_t const>::dshared_nullptr(), {}, call_context_depth, true, xml_output_format);
  //t->tfg_populate_relevant_memlabels(src_llvm_tfg);
  function_tfg_map->ftmap_add_start_pc_preconditions_for_each_tfg(/*se.m_srcdst_keyword*/);

  string llvm_header = M1->get_llvm_header_as_string();
  list<string> type_decls = M1->get_type_declarations_as_string();
  list<string> globals_with_initializers = M1->get_globals_with_initializers_as_string();
  list<string> function_decls = M1->get_function_declarations_as_string();
  map<string, function_signature_t> fname_signature_map = M1->get_function_signature_map();
  pair<map<string, llvm_fn_attribute_id_t>, map<llvm_fn_attribute_id_t, string>> fname_attributes_map = M1->get_function_attributes_map();
  map<string, link_status_t> fname_linker_status_map = M1->get_function_link_status_map();
  //map<llvm_fn_attribute_id_t, string> attributes = M1->get_attributes();
  //string llvm_id = M1->get_llvm_identifier();
  //string llvm_module_flags = M1->get_llvm_module_flags();
  list<string> llvm_metadata = M1->get_metadata_as_string();

  autostop_timer func2_timer(string(__func__) + ".2");
  llptfg_t llptfg(llvm_header, type_decls, globals_with_initializers, function_decls, *function_tfg_map, fname_signature_map, fname_attributes_map.first, fname_linker_status_map, fname_attributes_map.second, llvm_metadata);
  llptfg.print(outputStream);
  //for (auto ft : function_tfg_map) {
  //  string const &fname = ft.first;

  //  outputStream << "=FunctionName: " << fname << "\n";
  //  //outputStream << "=CalleeSummary:" << function_tfg_map.at(fname).first.callee_summary_to_string_for_eq() << "\n";
  //  outputStream << function_tfg_map.at(fname).second->tfg_to_string_for_eq() << "\n";
  //}
  autostop_timer func3_timer(string(__func__) + ".3");
  outputStream.close();
  outputStream.flush();

  //ofstream llcopy;
  //llcopy.open(OutputFilename + ".ll", ios_base::out | ios_base::trunc);
  //llptfg.output_llvm_code(llcopy);
  //llcopy.close();
  //llcopy.flush();

  CPP_DBG_EXEC2(STATS,
    print_all_timers();
    cout << stats::get() << endl;
  );
  call_on_exit_function();
  outs().flush();
  errs().flush();
  exit(0);


  //errs().changeColor(raw_ostream::RED);
  //errs() << " : " << (e.check_eq("out.proof") ? "passed" : "---------------failed????????")<< "\n";
  //errs().flush();
  

  return 0;
}
