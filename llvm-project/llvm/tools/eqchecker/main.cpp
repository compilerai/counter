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
#include "sym_exec_llvm.h"
#include "expr/consts_struct.h"
#include "expr/expr.h"
#include "eq/eqcheck.h"
#include "tfg/tfg_llvm.h"
#include "ptfg/llptfg.h"
#include "ptfg/function_signature.h"

#include "support/timers.h"
#include "support/dyn_debug.h"

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

static cl::opt<bool>
DisableModelingOfUninitVarUB("u", cl::desc("<disable-modeling-of-uninit-var-ub>"), cl::init(false));

static cl::opt<bool>
DryRun("dry-run", cl::desc("<dry-run. only print the function names and their sizes>"), cl::init(false));

static cl::opt<std::string>
DynDebug("dyn_debug", cl::desc("<debug.  enable dynamic debugging for debug-class(es).  Expects comma-separated list of debug-classes with optional level e.g. -debug=compute_liveness,sprels,alias_analysis=2"), cl::init(""));

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
  // Print a stack trace if we signal out.
  sys::PrintStackTraceOnErrorSignal(argv[0]);
  PrettyStackTraceProgram X(argc, argv);
  g_ctx_init();

  //LLVMContext &Context = getGlobalContext();
  LLVMContext Context;
  llvm_shutdown_obj Y;  // Call llvm_shutdown() on exit.

  //Context.setDiagnosticHandler(diagnosticHandler, argv[0]);

  cl::ParseCommandLineOptions(argc, argv, "llvm2tfg: file1.bc -> out.etfg\n");

  eqspace::init_dyn_debug_from_string(DynDebug);

  CPP_DBG_EXEC(LLVM2TFG, errs() << "doing functions:" << FunNames << "\n");
  CPP_DBG_EXEC(LLVM2TFG, errs() << "output filename:" << OutputFilename << "\n");

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
  CPP_DBG_EXEC(LLVM2TFG,
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
  CPP_DBG_EXEC(LLVM2TFG, errs() << InputFilename1 << "\n");
  //errs() << InputFilename2 << "\n";

  /*for(const pair<string, unsigned>& fun_name : fun_names)
  {
    errs() << fun_name.first << " : " << fun_name.second << "\n";
  }*/

  //context *ctx = new context(context::config(600, 600/*, true, true, true*/));
  context *ctx = g_ctx;
  ctx->parse_consts_db(CONSTS_DB_FILENAME);
  consts_struct_t &cs = ctx->get_consts_struct();

  ofstream outputStream;
  outputStream.open(OutputFilename, ios_base::out | ios_base::trunc);

  map<string, pair<callee_summary_t, unique_ptr<tfg_llvm_t>>> function_tfg_map;
  for (const Function& f : *M1) {
    if (   FunNamesVec.size() != 0
        //&& (FunNamesVec.size() != 1 || *FunNamesVec.begin() != "ALL")
        && !set_belongs(FunNamesVec, string(f.getName().data()))) {
      continue;
    }
    if (sym_exec_common::get_num_insn(f) == 0) {
      continue;
    }

    string fname = f.getName().str();
    autostop_timer total_timer(string(__func__));
    autostop_timer func_timer(string(__func__) + "." + fname);

    if (DryRun) {
      outputStream << fname << " : " << sym_exec_common::get_num_insn(f) << "\n";
      continue;
    }
    if (function_tfg_map.count(fname)) {
      continue;
    }
    //errs() << "Doing: " << fname << "\n";
    //errs().flush();

    bool gen_callee_summary = (FunNamesVec.size() == 0);
    set<string> function_call_chain;

    cout << __func__ << " " << __LINE__ << ": Doing " << fname << endl;
    unique_ptr<tfg_llvm_t> t_src = sym_exec_llvm::get_preprocessed_tfg(f, M1.get(), fname, ctx, function_tfg_map, function_call_chain, gen_callee_summary, DisableModelingOfUninitVarUB ? true : false);

    callee_summary_t csum = t_src->get_summary_for_calling_functions();
    function_tfg_map.insert(make_pair(fname, make_pair(csum, std::move(t_src))));
  }

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
  llptfg_t llptfg(llvm_header, type_decls, globals_with_initializers, function_decls, function_tfg_map, fname_signature_map, fname_attributes_map.first, fname_linker_status_map, fname_attributes_map.second, llvm_metadata);
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

  ofstream llcopy;
  llcopy.open(OutputFilename + ".ll", ios_base::out | ios_base::trunc);
  llptfg.output_llvm_code(llcopy);
  llcopy.close();
  llcopy.flush();

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
