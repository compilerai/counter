#include "dfa_helper.h"
#include "llvm/Pass.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/PassManager.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Analysis/CallGraphSCCPass.h"
#include "support/debug.h"
#include "llvm/Transforms/Utils/Cloning.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/IR/IRPrintingPasses.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"
#include "llvm/Analysis/CallGraph.h"
#include <sstream>

/*static string
get_basicblock_name(const BasicBlock& v)
{
  assert(isa<const BasicBlock>(v));

  string ret;
  raw_string_ostream ss(ret);
  v.printAsOperand(ss, false);
  ss.flush();

  ASSERT(ret.substr(0, 1) == "%");
  ret = ret.substr(1);
  return ret;
}*/

string
get_basicblock_name(const llvm::BasicBlock& v)
{
  assert(isa<const BasicBlock>(v));

  string ret;
  raw_string_ostream ss(ret);
  v.printAsOperand(ss, false);
  ss.flush();

  return ret;
}

int
get_counting_index_for_basicblock(llvm::BasicBlock const& v)
{
  int bbnum = 0;
  for (const BasicBlock& B : *v.getParent()) {
    if (&B == &v) {
      return bbnum;
    }
    bbnum++;
  }
  NOT_REACHED();
}

unique_ptr<tfg_llvm_t>
function2tfg(Function *F, Module *M, map<shared_ptr<tfg_edge const>, Instruction *>& eimap)
{
  if (!g_ctx) {
    g_ctx_init();
  }
  if (!F && !M) {
    return nullptr;
  }
  context *ctx = g_ctx;
  ValueToValueMapTy VMap;
  //unique_ptr<Module> Mcopy = CloneModule(*M, VMap);
  sym_exec_llvm se(ctx, M, *F, false, BYTE_LEN, DWORD_LEN);
  unique_ptr<tfg_llvm_t> ret = se.get_tfg(nullptr, nullptr, eimap);
  pc start_pc = se.get_start_pc();
  ret->add_extra_node_at_start_pc(start_pc);
  //cout << ret->tfg_to_string_for_eq() << "\n";
  return ret;
}

static string global_function_name;

void
set_global_function_name(string const& fname)
{
  global_function_name = fname;
}

string const&
get_global_function_name()
{
  return global_function_name;
}
