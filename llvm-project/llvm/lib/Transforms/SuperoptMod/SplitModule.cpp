#include "llvm/Pass.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/PassManager.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Analysis/CallGraphSCCPass.h"
#include "support/debug.h"
#include "support/utils.h"
#include "llvm/Transforms/Utils/Cloning.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/IR/IRPrintingPasses.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"
#include "llvm/Analysis/CallGraph.h"
#include "llvm/InitializePasses.h"
#include <sstream>

using namespace llvm;
using namespace std;

namespace {

class SplitModule : public CallGraphSCCPass {
private:
  Module const *M;
  string m_function_with_globals; //the function with whose definition we will emit the globals
  int m_counter;
public:
  static char ID;
  SplitModule() : CallGraphSCCPass(ID), M(nullptr), m_function_with_globals(), m_counter(-1)
  { }
  //SplitModule(Module *m) : CallGraphSCCPass(ID), M(m), m_main_function(identifyMainFunction(M)), m_counter(0)
  //{ }

  virtual bool doInitialization(CallGraph &CG) override
  {
    M = &CG.getModule();
    m_function_with_globals = identifyFunctionWithGlobals(M);
    m_counter = 0;
    return false;
  }

  virtual bool runOnSCC(CallGraphSCC &SCC) override;

  //virtual bool doFinalization(CallGraph &CG);
private:
  //Module *makeLLVMModule(CallGraphSCC const& SCC) const;
  bool selectGVsToClone(GlobalValue const *gv, CallGraphSCC const& SCC) const;
  static string identifyFunctionWithGlobals(Module const* M);
};

bool
SplitModule::selectGVsToClone(GlobalValue const *gv, CallGraphSCC const& SCC) const
{
  bool function_with_globals_found = false;
  Function const *gvF = dyn_cast<Function>(gv);
  for (auto CGnode : SCC) {
    Function *F = CGnode->getFunction();
    if (!F) {
      continue;
    }
    if (F->isDeclaration()) {
      continue;
    }
    StringRef fname_ref = F->getName();
    string fname = fname_ref.data();
    if (fname == m_function_with_globals) {
      function_with_globals_found = true;
    }
    if (gvF) {
      if (fname == gvF->getName().data()) {
        //dbgs() << __func__ << " " << __LINE__ << ": returning true for " << fname << " vs. " << gvF->getName().data() << "\n";
        return true;
      } else {
        //dbgs() << __func__ << " " << __LINE__ << ": could not match " << fname << " vs. " << gvF->getName().data() << "\n";
      }
    }
  }
  if (gvF) {
    return false;
  } else {
    return function_with_globals_found;
  }
}

string
SplitModule::identifyFunctionWithGlobals(Module const* M)
{
  auto iter = M->begin();
  ASSERT(iter != M->end());
  Function const& F = *iter;
  return F.getName().str();  //return the name of the first function; this is the function whose compilation context will contain the globals
}

bool
SplitModule::runOnSCC(CallGraphSCC &SCC)
{
  ValueToValueMapTy VMap;
  std::function<bool (GlobalValue const*)> f = [this, &SCC](GlobalValue const* GV)
  {
    return this->selectGVsToClone(GV, SCC);
  };
  unique_ptr<Module> new_M = CloneModule(*M, VMap, f);
  if (new_M->empty()) {
    return false;
  }
  int num_defined_functions = 0;
  for (auto const& F : *new_M) {
    if (!F.isDeclaration()) {
      outs() << __FILE__ << " " << __func__ << " " << __LINE__ << ": defined function exists in split." << m_counter << ": " << F.getName() << "\n";
      num_defined_functions++;
    }
  }
  if (num_defined_functions == 0) {
    return false;
  }
  std::error_code EC;

  string module_name = M->getName().data();
  stringstream ss;
  ASSERT(string_has_suffix(module_name, ".bc"));
  ss << module_name.substr(0, module_name.size() - 3) << ".split" << m_counter << ".ll";
  m_counter++;
  raw_fd_ostream fp(ss.str().c_str(), EC, sys::fs::F_Text);
  legacy::PassManager PM;
  PM.add(createPrintModulePass(fp));
  //outs() << "========================SCC===================\n";
  PM.run(*new_M);

  return false; //have not modified the original SCC
}

//Module *
//SplitModule::makeLLVMModule(CallGraphSCC const& SCC) const
//{
//  CallGraph const& CG = SCC.getCallGraph();
//  CallGraphNode const *first_node = *CG.begin();
//  Function *firstF = first_node->getFunction();
//  string first_fname = firstF->getName().data();
//  Module* new_mod = new Module(first_fname, getGlobalContext());
//  for (auto CGnode : CG) {
//    Function *F = CGnode->getFunction();
//    string fname = F->getName().data();
//    FunctionCallee newFCallee = new_mod->getOrInsertFunction(fname, F->getFunctionType());
//    Value *callee = newFCallee.getCallee();
//    NOT_IMPLEMENTED();
//  }
//}

}

char SplitModule::ID = 0;

INITIALIZE_PASS_BEGIN(SplitModule, "splitmodule",
                      "Split Module into multiple SCC modules", false,
                      false)
INITIALIZE_PASS_END(SplitModule, "splitmodule",
                    "Split Module into multiple SCC modules", false, false)
static RegisterPass<SplitModule> X("splitModule", "Split one module into per-SCC modules");

static RegisterStandardPasses Y(
    PassManagerBuilder::EP_EarlyAsPossible,
    [](PassManagerBuilder const&  Builder,
       legacy::PassManagerBase &PM) { PM.add(new SplitModule()); });
