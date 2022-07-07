#pragma once

#include <fstream>
#include <functional>
#include <map>
#include <string>
#include "boost/optional.hpp"

#include "yaml-cpp/yaml.h"

#include "expr/axpred_parser.h"
#include "expr/defs.h"
#include "expr/expr.h"
#include "expr/proof_status.h"

namespace eqspace {
namespace axpred {

using SubstTy = std::map<std::string, expr_ref>;

class AxPredContext {
private:
  context *mContext;
  AxPreds mAxs;
  mutable std::ofstream mDebugOS;

public:
  AxPredContext(context *Context, const std::string &AxsFile);
  AxPredContext(context *Context, const std::string &AxsFile,
                const std::string &DebugFile);

  context &getContext();
  std::vector<expr_ref> getPrecondsForAxPred(const expr_ref &Ax) const;
  std::vector<expr_ref> getAllPrecondExprs(const expr_ref &Expr,
                                           bool IncludeItself);

  std::ofstream &debug() const;

  solver_res attemptUnifySolve(
      const std::function<bool(const expr_ref &Expr)> &ExprEqSolver,
      const std::function<void(const expr_ref &Expr, std::size_t CurDepth,
                               std::size_t MaxDepth, bool)> &Trace,
      const expr_ref &Expr, std::size_t MaxDepth);

private:
  void init(const std::string &AxsFile);

  // Returns S such that Concl[S] = AxExpr
  optional<SubstTy> computeEqualizerSubst(expr_ref Concl,
                                          expr_ref AxExpr) const;
  bool computeEqualizerSubstImpl(expr_ref Concl, expr_ref AxExpr,
                                 SubstTy &Subst) const;

  solver_res attemptUnifySolveImpl(
      const std::function<bool(const expr_ref &Expr)> &ExprEqSolver,
      const std::function<void(const expr_ref &Expr, std::size_t CurDepth,
                               std::size_t MaxDepth, bool)> &Trace,
      const expr_ref &Expr, std::size_t CurDepth, std::size_t MaxDepth);
};

class AxPredPrecondsVisitor : public expr_visitor {
private:
  AxPredContext &mAxPredContext;
  std::map<expr_id_t, std::vector<expr_ref>> mPreconds;
  expr_ref mExpr;
  bool mContainsAxPreds = false;

public:
  AxPredPrecondsVisitor(AxPredContext &AxPredContext, const expr_ref &Expr);

  std::vector<expr_ref> getAllPrecondExprs() const;
  bool containsAxPreds() const;

  virtual void previsit(const expr_ref &Expr, int *IntArgs,
                        int &NumIntArgs) override;
  virtual void visit(const expr_ref &Expr) override;

private:
  std::vector<expr_ref>
  getAllCombs(expr::operation_kind Op,
              const std::vector<std::vector<expr_ref> *> &Args);

  void getAllCombsImpl(expr::operation_kind Op,
                       const std::vector<std::vector<expr_ref> *> &Args,
                       std::vector<expr_ref> &Comb, std::vector<expr_ref> &Ret,
                       size_t Index);
};

} // namespace axpred
} // namespace eqspace
