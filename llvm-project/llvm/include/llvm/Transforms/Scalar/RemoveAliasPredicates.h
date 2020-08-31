#ifndef LLVM_TRANSFORMS_SCALAR_REMOVEALIASPREDICATES_H
#define LLVM_TRANSFORMS_SCALAR_REMOVEALIASPREDICATES_H

#include "llvm/IR/Function.h"
#include "llvm/IR/PassManager.h"

namespace llvm {

class RemoveAliasPredsPass : public PassInfoMixin<RemoveAliasPredsPass> {
public:
  PreservedAnalyses run(Function &F, FunctionAnalysisManager &AM);
};
}

#endif

