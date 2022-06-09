#ifndef LLVM_ANALYSIS_SEMANTICALIASANALYSIS_H
#define LLVM_ANALYSIS_SEMANTICALIASANALYSIS_H

#include "llvm/ADT/DenseMap.h"
#include "llvm/Analysis/AliasAnalysis.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Module.h"
#include "llvm/Pass.h"
#include "tfg/tfg_llvm.h"
#include "ptfg/llvm_value_id.h"
#include "ptfg/ftmap.h"

#include <set>

namespace llvm {

class SemanticAAResult : public AAResultBase<SemanticAAResult> {
public:
  //using function_tfg_map_t = map<string, pair<callee_summary_t, dshared_ptr<tfg_llvm_t>>>;
  using function_tfg_map_t = ftmap_t;
private:
  shared_ptr<function_tfg_map_t const> m_function_tfg_map;
  shared_ptr<map<llvm_value_id_t, string_ref> const> m_value_to_name_map;
private:
  std::string memory_location_get_name(MemoryLocation const& l) const;
  static llvm_value_id_t get_llvm_value_id_for_value(Value const* v);
public:
  explicit SemanticAAResult(shared_ptr<function_tfg_map_t const> const& function_tfg_map, shared_ptr<map<llvm_value_id_t, string_ref> const> const& value_to_name_map) : AAResultBase(), m_function_tfg_map(function_tfg_map), m_value_to_name_map(value_to_name_map) {}
  SemanticAAResult(SemanticAAResult &&Arg)
      : AAResultBase(std::move(Arg)), m_function_tfg_map(Arg.m_function_tfg_map), m_value_to_name_map(Arg.m_value_to_name_map) {}

  AliasResult alias(const MemoryLocation &LocA, const MemoryLocation &LocB, AAQueryInfo &AAQI);
  static AliasResult convertTfgAliasResultToAliasResult(tfg_alias_result_t tfg_alias_result);
  static std::string get_function_name(Value const * ptrA, Value const * ptrB);
};

/// Analysis pass providing a never-invalidated alias analysis result.
class SemanticAA : public AnalysisInfoMixin<SemanticAA> {
  friend AnalysisInfoMixin<SemanticAA>;
  static AnalysisKey Key;

public:
  typedef SemanticAAResult Result;

  SemanticAAResult run(Function &F, FunctionAnalysisManager &AM);
};

/// Legacy wrapper pass to provide the SemanticAAResult object.
class SemanticAAWrapperPass : public ImmutablePass {
  std::unique_ptr<SemanticAAResult> Result;

public:
  static char ID;

  SemanticAAWrapperPass();

  SemanticAAResult &getResult() { return *Result; }
  const SemanticAAResult &getResult() const { return *Result; }

  bool doInitialization(Module &M) override;
  bool doFinalization(Module &M) override;
  void getAnalysisUsage(AnalysisUsage &AU) const override;
};

/// Creates an instance of \c SemanticAAWrapperPass.
ImmutablePass *createSemanticAAWrapperPass();
}

#endif
