#include "llvm/Pass.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/PassManager.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Analysis/CallGraphSCCPass.h"
#include "support/debug.h"
#include "support/dyn_debug.h"
#include "support/utils.h"
#include "llvm/Transforms/Utils/Cloning.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/IR/IRPrintingPasses.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"
#include "llvm/Analysis/CallGraph.h"
#include "llvm/InitializePasses.h"
#include <sstream>

using namespace llvm;
using namespace eqspace;
using namespace std;

namespace {

class ConvertLocalLinkageToGlobal : public FunctionPass {
private:
  Module *M;
public:
  static char ID;
  ConvertLocalLinkageToGlobal() : FunctionPass(ID), M(nullptr)
  {
    DYN_DEBUG(convertLocalLinkageToGlobal,
        dbgs() << "ConvertLocalLinkageToGlobal() constructor: ID = " << int(ID) << ", getPassID() = " << this->getPassID() << "\n";
    );
  }

  void getAnalysisUsage(AnalysisUsage &AU) const override
  {
    DYN_DEBUG(convertLocalLinkageToGlobal,
        dbgs() << "ConvertLocalLinkageToGlobal() getAnalysisUsage: ID = " << int(ID) << ", getPassID() = " << this->getPassID() << "\n";
    );
    AU.setPreservesAll();
  }

  virtual bool doInitialization(Module& m) override
  {
    M = &m;
    return false;
  }

  virtual bool runOnFunction(Function &F) override;

  StringRef getPassName() const { return "Convert functions with local linkage to global linkage"; }
private:
};

bool
ConvertLocalLinkageToGlobal::runOnFunction(Function &F)
{
  if (F.hasLocalLinkage()) {
    dbgs() << __FILE__ << " " << __func__ << " " << __LINE__ << ": " << F.getName().str() << " has local linkage\n";
    F.setLinkage(GlobalValue::ExternalLinkage);
    return true;
  }
  return false;
}

}

char ConvertLocalLinkageToGlobal::ID = 0;

//INITIALIZE_PASS_BEGIN(ConvertLocalLinkageToGlobal, "convertLocalLinkageToGlobal",
//                      "Convert functions and globals with local linkage to global linkage", false,
//                      false)
//INITIALIZE_PASS_END(ConvertLocalLinkageToGlobal, "convertLocalLinkageToGlobal",
//                    "Convert functions and globals with local linkage to global linkage", false,
//                    false)

static RegisterPass<ConvertLocalLinkageToGlobal> Local2Global("convertLocalLinkageToGlobal", "Convert functions and globals with local linkage to global linkage");
