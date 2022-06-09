#include "llvm/Analysis/SemanticAliasAnalysis.h"

#include "llvm/IR/InstIterator.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Intrinsics.h"
#include "llvm/InitializePasses.h"
#include "Superopt/dfa_helper.h"
#include "Superopt/sym_exec_llvm.h"

#include "tfg/tfg_llvm.h"
#include "ptfg/llvm_value_id.h"
#include "ptfg/llptfg.h"

using namespace llvm;

AliasResult
SemanticAAResult::convertTfgAliasResultToAliasResult(tfg_alias_result_t tfg_alias_result)
{
  switch (tfg_alias_result) {
    case TfgMayAlias: {
      return MayAlias;
    }
    case TfgMustNotAlias: {
      return NoAlias;
    }
    case TfgMustAlias: {
      return MustAlias;
    }
    case TfgPartialAlias: {
      return PartialAlias;
    }
    default: NOT_REACHED();
  }
}

#ifndef NDEBUG
static bool notDifferentParent(const Value *O1, const Value *O2) {

  const Function *F1 = sym_exec_llvm::getParent(O1);
  const Function *F2 = sym_exec_llvm::getParent(O2);

  return !F1 || !F2 || F1 == F2;
}
#endif

string
SemanticAAResult::memory_location_get_name(MemoryLocation const& l) const
{
  llvm_value_id_t llvm_value_id = sym_exec_llvm::get_llvm_value_id_for_value(l.Ptr);
  DYN_DEBUG2(aliasAnalysis, std::cout << "SemanticAAResult::" << __func__ << " " << __LINE__ << ": llvm_value_id = " << llvm_value_id.llvm_value_id_to_string() << "\n");
  if (m_value_to_name_map && m_value_to_name_map->count(llvm_value_id)) {
    return m_value_to_name_map->at(llvm_value_id)->get_str();
  }
  return sym_exec_common::get_value_name_using_srcdst_keyword(*l.Ptr, G_SRC_KEYWORD);
}


AliasResult
SemanticAAResult::alias(const MemoryLocation &LocA,
                        const MemoryLocation &LocB,
                        AAQueryInfo &AAQI) {
  string nameA = memory_location_get_name(LocA);
  string nameB = memory_location_get_name(LocB);
  DYN_DEBUG2(aliasAnalysis, std::cout << "SemanticAAResult::" << __func__ << " " << __LINE__ << ": LocA = " << nameA << "\n");
  DYN_DEBUG2(aliasAnalysis, std::cout << "SemanticAAResult::" << __func__ << " " << __LINE__ << ": LocB = " << nameB << "\n");

  AliasResult retval;
  bool should_return_immediately = false;
  DYN_DEBUG_MUTE(disableSemanticAA,
    // Forward the query to the next analysis.
    retval = AAResultBase::alias(LocA, LocB, AAQI);
    should_return_immediately = true; //to avoid control flow from inside the macro, simply set a flag
  );
  if (should_return_immediately) {
    return retval;
  }

  assert(notDifferentParent(LocA.Ptr, LocB.Ptr) &&
         "SemanticAliasAnalysis doesn't support interprocedural queries.");

  Function const* F1 = sym_exec_llvm::getParent(LocA.Ptr);
  Function const* F2 = sym_exec_llvm::getParent(LocB.Ptr);
  Function const* F = nullptr;

  if (F1) {
    F = F1;
  }
  if (F2) {
    F = F2;
  }

  string fname = F ? F->getName().str() : "";

  DYN_DEBUG2(aliasAnalysis, std::cout << "SemanticAAResult::" << __func__ << " " << __LINE__ << ": LocA = " << nameA << "\n");
  DYN_DEBUG2(aliasAnalysis, std::cout << "SemanticAAResult::" << __func__ << " " << __LINE__ << ": LocB = " << nameB << "\n");

  uint64_t sizeA = LocA.Size.isPrecise() ? LocA.Size.getValue() : SEMANTICAA_LOCSIZE_UNKNOWN;
  uint64_t sizeB = LocB.Size.isPrecise() ? LocB.Size.getValue() : SEMANTICAA_LOCSIZE_UNKNOWN;

  return convertTfgAliasResultToAliasResult(m_function_tfg_map->ftmap_get_aliasing_relationship_between_memaccesses(fname, nameA, sizeA, nameB, sizeB));

  // Check if there is a predicate corresponding to LocA and LocB
  //if ((predicates.count(LocA.Ptr) && predicates[LocA.Ptr].count(LocB.Ptr)) ||
  //    (predicates.count(LocB.Ptr) && predicates[LocB.Ptr].count(LocA.Ptr))) {
  //  return NoAlias;
  //}

}

char SemanticAAWrapperPass::ID = 0;
INITIALIZE_PASS(SemanticAAWrapperPass, "semantic-aa", "Semantic Alias Analysis",
                false, true)

ImmutablePass *llvm::createSemanticAAWrapperPass() {
  return new SemanticAAWrapperPass();
}

SemanticAAWrapperPass::SemanticAAWrapperPass() : ImmutablePass(ID) {
  initializeSemanticAAWrapperPassPass(*PassRegistry::getPassRegistry());
}

bool SemanticAAWrapperPass::doInitialization(Module &M)
{
  DYN_DEBUG(aliasAnalysis, std::cout << "SemanticAAResult::doInitialization() called\n");
  bool should_return_immediately = false;
  DYN_DEBUG_MUTE(disableSemanticAA,
    Result.reset(new SemanticAAResult(nullptr, nullptr));
    should_return_immediately = true; //to avoid control flow from inside the macro, simply set a flag
  );
  if (should_return_immediately) {
    return false;
  }
  //string const& fname = F.getName().str();
  //if (m_function_tfg_map->count(fname)) {
  //  return false;
  //}
  //Module &M = *F.getParent();
  //map<shared_ptr<tfg_edge const>, Instruction *> eimap;
  //DYN_DEBUG(llvm2tfg, std::cout << "SemanticAAWrapperPass::" << __func__ << " " << __LINE__ << ": F.getName() = " << F.getName() << "\n");
  if (!g_ctx) {
    g_ctx_init();
  }
  ASSERT(g_ctx);

  map<llvm_value_id_t, string_ref> value_to_name_map;
  shared_ptr<SemanticAAResult::function_tfg_map_t const> function_tfg_map = make_shared<SemanticAAResult::function_tfg_map_t const>(*sym_exec_llvm::get_function_tfg_map(&M, set<string>()/*, false*/, g_ctx, dshared_ptr<llptfg_t const>::dshared_nullptr(), false, false /*model_llvm_semantics*/, &value_to_name_map));
  Result.reset(new SemanticAAResult(function_tfg_map, make_shared<map<llvm_value_id_t, string_ref> const>(value_to_name_map)));
  return false;
}

bool SemanticAAWrapperPass::doFinalization(Module &M)
{
  return false;
}

void SemanticAAWrapperPass::getAnalysisUsage(AnalysisUsage &AU) const {
  AU.setPreservesAll();
}

std::string
SemanticAAResult::get_function_name(Value const * ptrA, Value const * ptrB)
{
  Function const* fA = sym_exec_llvm::getParent(ptrA);
  if (fA) {
    return fA->getName().str();
  }
  Function const* fB = sym_exec_llvm::getParent(ptrB);
  if (fB) {
    return fB->getName().str();
  }
  return FNAME_GLOBAL_SPACE;
/*
  Instruction const* iA = dyn_cast<Instruction>(ptrA);
  if (iA) {
    BasicBlock const* bb = iA->getParent();
    ASSERT(bb);
    Function const* f = bb->getParent();
    return f->getName().str();
  }
  Instruction const* iB = dyn_cast<Instruction>(ptrB);
  if (iB) {
    BasicBlock const* bb = iB->getParent();
    ASSERT(bb);
    Function const* f = bb->getParent();
    return f->getName().str();
  }
  return "<global>";
*/
}
