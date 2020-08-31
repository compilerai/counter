#ifndef LLVM_ANALYSIS_UNSEQUENCEDALIASANALYSIS_H
#define LLVM_ANALYSIS_UNSEQUENCEDALIASANALYSIS_H

#include "llvm/ADT/DenseMap.h"
#include "llvm/Analysis/AliasAnalysis.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Module.h"
#include "llvm/Pass.h"

#include <set>

namespace llvm {

typedef DenseMap<const Value *, std::set<const Value *>> PREDICATE_MAP;

class UnseqAAResult : public AAResultBase<UnseqAAResult> {
  PREDICATE_MAP predicates;

public:
  explicit UnseqAAResult(PREDICATE_MAP &pm) : AAResultBase(), predicates(pm) {}
  UnseqAAResult(UnseqAAResult &&Arg)
      : AAResultBase(std::move(Arg)), predicates(Arg.predicates) {}

  AliasResult alias(const MemoryLocation &LocA, const MemoryLocation &LocB, AAQueryInfo &AAQI);
};

/// Analysis pass providing a never-invalidated alias analysis result.
class UnseqAA : public AnalysisInfoMixin<UnseqAA> {
  friend AnalysisInfoMixin<UnseqAA>;
  static AnalysisKey Key;

public:
  typedef UnseqAAResult Result;

  UnseqAAResult run(Function &F, FunctionAnalysisManager &AM);
};

/// Legacy wrapper pass to provide the UnseqAAResult object.
class UnseqAAWrapperPass : public FunctionPass {
  std::unique_ptr<UnseqAAResult> Result;

public:
  static char ID;

  UnseqAAWrapperPass();

  UnseqAAResult &getResult() { return *Result; }
  const UnseqAAResult &getResult() const { return *Result; }

  bool runOnFunction(Function &F) override;
  void getAnalysisUsage(AnalysisUsage &AU) const override;
};

/// Creates an instance of \c UnseqAAWrapperPass.
FunctionPass *createUnseqAAWrapperPass();
}

#endif
