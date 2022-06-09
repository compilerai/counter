#include "llvm/Transforms/Scalar/RemoveAliasPredicates.h"
#include "llvm/IR/InstIterator.h"
#include "llvm/IR/Instructions.h"
#include "llvm/Transforms/Scalar.h"
#include "llvm/InitializePasses.h"

#include <iostream>
#include <vector>

using namespace llvm;

static bool removePredicates(Function &F) {
  // Start out with all of the instructions in the worklist...
  std::vector<Instruction*> WorkList;
  for (inst_iterator i = inst_begin(F), e = inst_end(F); i != e; ++i)
    WorkList.push_back(&*i);

  // Loop over the worklist finding instructions that are dead.  If they are
  // dead make them drop all of their uses, making other instructions
  // potentially dead, and work until the worklist is empty.
  bool MadeChange = false;
  while (!WorkList.empty()) {
    Instruction* I = WorkList.back();
    WorkList.pop_back();
    if (isa<CallInst>(I)) {
      Value* sv = cast<CallInst>(I)->getCalledOperand()->stripPointerCasts();
      if (sv->getName() == "__not_alias") {
        // Remove the instruction.
        I->eraseFromParent();

        // Remove the instruction from the worklist if it still exists in it.
        WorkList.erase(std::remove(WorkList.begin(), WorkList.end(), I), WorkList.end());
        MadeChange = true;
      }
    }
  }
  return MadeChange;
}

PreservedAnalyses RemoveAliasPredsPass::run(Function &F, FunctionAnalysisManager &AM) {
  if (!removePredicates(F))
    return PreservedAnalyses::all();

  PreservedAnalyses PA;
  PA.preserveSet<CFGAnalyses>();
  return PA;
}

namespace {
struct RemoveAliasPredsLegacyPass : public FunctionPass {
  static char ID; // Pass identification, replacement for typeid
  RemoveAliasPredsLegacyPass() : FunctionPass(ID) {
    initializeRemoveAliasPredsLegacyPassPass(*PassRegistry::getPassRegistry());
  }

  bool runOnFunction(Function &F) override {
    if (skipFunction(F))
      return false;

    return removePredicates(F);
  }

  void getAnalysisUsage(AnalysisUsage &AU) const override {
    AU.setPreservesCFG();
  }
};
}

char RemoveAliasPredsLegacyPass::ID = 0;
INITIALIZE_PASS(RemoveAliasPredsLegacyPass, "rem-preds", "Remove alias predicates", false, false)

FunctionPass *llvm::createRemoveAliasPredicatesPass() {
  return new RemoveAliasPredsLegacyPass();
}

