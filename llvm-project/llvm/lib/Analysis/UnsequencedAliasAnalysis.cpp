#include "llvm/Analysis/UnsequencedAliasAnalysis.h"

#include "llvm/IR/InstIterator.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Intrinsics.h"
#include "llvm/InitializePasses.h"

#include "Superopt/sym_exec_llvm.h"

using namespace llvm;

AliasResult UnseqAAResult::alias(const MemoryLocation &LocA,
                                 const MemoryLocation &LocB,
                                 AAQueryInfo &AAQI) {

  DYN_DEBUG2(aliasAnalysis, std::cout << "UnseqAAResult::" << __func__ << " " << __LINE__ << ": LocA = " << sym_exec_common::get_value_name_using_srcdst_keyword(*LocA.Ptr, G_SRC_KEYWORD) << "\n");
  DYN_DEBUG2(aliasAnalysis, std::cout << "UnseqAAResult::" << __func__ << " " << __LINE__ << ": LocB = " << sym_exec_common::get_value_name_using_srcdst_keyword(*LocB.Ptr, G_SRC_KEYWORD) << "\n");
  // Check if there is a predicate corresponding to LocA and LocB
  if ((predicates.count(LocA.Ptr) && predicates[LocA.Ptr].count(LocB.Ptr)) ||
      (predicates.count(LocB.Ptr) && predicates[LocB.Ptr].count(LocA.Ptr))) {
    return NoAlias;
  }

  // Forward the query to the next analysis.
  return AAResultBase::alias(LocA, LocB, AAQI);
}

char UnseqAAWrapperPass::ID = 0;
INITIALIZE_PASS(UnseqAAWrapperPass, "unseq-aa", "Unsequenced Alias Analysis",
                false, true)

FunctionPass *llvm::createUnseqAAWrapperPass() {
  return new UnseqAAWrapperPass();
}

UnseqAAWrapperPass::UnseqAAWrapperPass() : FunctionPass(ID) {
  initializeUnseqAAWrapperPassPass(*PassRegistry::getPassRegistry());
}

Value *getValuefromMDValue(Value *v) {
  auto *mdn = cast<MetadataAsValue>(v)->getMetadata();
  if (auto *val = dyn_cast<ValueAsMetadata>(mdn)) {
    return val->getValue();
  }
  return nullptr;
}

bool UnseqAAWrapperPass::runOnFunction(Function &F) {
  PREDICATE_MAP predicates;
  for (inst_iterator it = inst_begin(F), e = inst_end(F); it != e; ++it) {
#define CALL cast<CallInst>(*it)
    if (isa<CallInst>(*it) &&
        CALL.getIntrinsicID() == Intrinsic::unseq_noalias) {

      // check if the predicate has any non-readonly fn calls assoc with it
      // The first operand of the predicate will be the unique id
      bool keepPred = true;
      for (unsigned i = 3; i < CALL.getNumArgOperands(); i++) {
        Value *val = getValuefromMDValue(CALL.getArgOperand(i));
        if (!val) {
          // llvm::errs() << "Null function value in MDNode\n";
          keepPred = false;
          continue;
        }

        if (Function *fn = dyn_cast<Function>(val)) {
          keepPred &= fn->doesNotAccessMemory();
        }
      }

      if (keepPred) {
        Value *mdArg1 = CALL.getArgOperand(1), *mdArg2 = CALL.getArgOperand(2),
              *vArg1, *vArg2;
        if ((vArg1 = getValuefromMDValue(mdArg1)) &&
            (vArg2 = getValuefromMDValue(mdArg2))) {
          predicates[vArg1].insert(vArg2);
        }
      }
    }
  }

  Result.reset(new UnseqAAResult(predicates));
  return false;
}

void UnseqAAWrapperPass::getAnalysisUsage(AnalysisUsage &AU) const {
  AU.setPreservesAll();
}
