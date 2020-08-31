//===- PrintFunctionNames.cpp ---------------------------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// Plugin to perform AST analysis for unsequenced races
//
//===----------------------------------------------------------------------===//
#include "UnsequencedAliasVisitor.h"

#include <unordered_set>

#include "llvm/ADT/Statistic.h"

using namespace clang;

#define DEBUG_TYPE "unseq"
STATISTIC(NUM_PREDICATES, "Number of no alias predicates generated");
STATISTIC(NUM_FUNC_PREDS,
          "Number of no alias predicates affected by function calls");
STATISTIC(FULL_EXPRS,
          "Number of full expressions that generate alias predicates");

std::string getFileNameFromPath(std::string path) {
  std::reverse(path.begin(), path.end());
  std::string filename = path;

  auto pos = path.find("/");
  if (pos != std::string::npos)
    filename = path.substr(0, pos);

  std::reverse(filename.begin(), filename.end());
  return filename;
}

std::string printExpr(const Expr *E) {
  E = E->IgnoreParenCasts();
  if (const UnaryOperator *UO = dyn_cast<UnaryOperator>(E)) {
    std::string O = printExpr(UO->getSubExpr());
    if (UO->getOpcode() == UO_Deref && !O.empty()) {
      return "*(" + O + ")";
    }
    if (UO->getOpcode() == UO_AddrOf && !O.empty()) {
      return "&(" + O + ")";
    }
    if (UO->getOpcode() == UO_PreInc) {
      return "(" + O + " + 1)";
    }
    if (UO->getOpcode() == UO_PreDec) {
      return "(" + O + " - 1)";
    }
    return O;
  } else if (const BinaryOperator *BO = dyn_cast<BinaryOperator>(E)) {
    return ("(" + printExpr(BO->getLHS()) + " " + BO->getOpcodeStr().str() +
            " " + printExpr(BO->getRHS()) + ")");
  } else if (const ArraySubscriptExpr *ASE = dyn_cast<ArraySubscriptExpr>(E)) {
    return printExpr(ASE->getLHS()) + "[" + printExpr(ASE->getRHS()) + "]";
  } else if (const MemberExpr *ME = dyn_cast<MemberExpr>(E)) {
    std::string base = printExpr(ME->getBase());
    base += ME->isArrow() ? "->" : ".";
    return "(" + base + ME->getMemberDecl()->getName().str() + ")";
  } else if (const DeclRefExpr *DRE = dyn_cast<DeclRefExpr>(E)) {
    return DRE->getDecl()->getName().str();
  } else if (const IntegerLiteral *IL = dyn_cast<IntegerLiteral>(E)) {
    int value = (IL->getValue()).getLimitedValue();
    return std::to_string(value);
    ;
  }
  return "";
}

void printAliasRules(clang::PREDICATE_MAP &predicateMap, std::string &filename,
                     clang::SourceManager &SM) {
  if (predicateMap.empty())
    return;

  std::error_code ec;
  llvm::raw_fd_ostream fout(filename, ec, llvm::sys::fs::OpenFlags::F_None);
  fout << "--------------------------------------------------------------------"
          "--"
       << "\n"
       << "\n";
  for (auto it1 = predicateMap.begin(); it1 != predicateMap.end(); it1++) {
    auto loc = it1->first->getExprLoc();
    std::string key = SM.getFilename(loc).str() + ":" +
                      std::to_string(SM.getSpellingLineNumber(loc));
    fout << "SEQ - " << key << '\n';

    auto &objSet = it1->second;
    for (auto it2 = objSet.begin(); it2 != objSet.end(); it2++) {
      fout << printExpr((*it2)[0]) << " and " << printExpr((*it2)[1])
           << " cannot alias"
           << "\n";
    }
    fout << "\n";
  }
  fout << "--------------------------------------------------------------------"
          "--"
       << "\n";
  fout << "\n";
}

namespace clang {

void SeqConsumer::PrintStats() {
  printAliasRules(*(visitor_.predicateMap), OutFileName_, SourceManager_);
}

bool SeqConsumer::HandleTopLevelDecl(DeclGroupRef d) {
  DeclGroupRef::iterator it;
  for (it = d.begin(); it != d.end(); it++) {
    visitor_.TraverseDecl(*it);
  }
  return true;
}

class SequenceChecker : public EvaluatedExprVisitor<SequenceChecker> {

  using Base = EvaluatedExprVisitor<SequenceChecker>;

  /// A tree of sequenced regions within an expression. Two regions are
  /// unsequenced if one is an ancestor or a descendent of the other. When we
  /// finish processing an expression with sequencing, such as a comma
  /// expression, we fold its tree nodes into its parent, since they are
  /// unsequenced with respect to nodes we will visit later.
  class SequenceTree {
    struct Value {
      explicit Value(unsigned Parent) : Parent(Parent), Merged(false) {}
      unsigned Parent : 31;
      unsigned Merged : 1;
    };
    SmallVector<Value, 8> Values;

  public:
    /// A region within an expression which may be sequenced with respect
    /// to some other region.
    class Seq {
      friend class SequenceTree;
      unsigned Index = 0;
      explicit Seq(unsigned N) : Index(N) {}

    public:
      Seq() = default;
      unsigned getIndex() { return Index; }
    };

    SequenceTree() { Values.push_back(Value(0)); }
    Seq root() const { return Seq(0); }

    /// Create a new sequence of operations, which is an unsequenced
    /// subset of \p Parent. This sequence of operations is sequenced with
    /// respect to other children of \p Parent.
    Seq allocate(Seq Parent) {
      Values.push_back(Value(Parent.Index));
      return Seq(Values.size() - 1);
    }

    /// Merge a sequence of operations into its parent.
    void merge(Seq S) { Values[S.Index].Merged = true; }

    /// Determine whether two operations are unsequenced. This operation
    /// is asymmetric: \p Cur should be the more recent sequence, and \p Old
    /// should have been merged into its parent as appropriate.
    bool isUnsequenced(Seq Cur, Seq Old) {
      unsigned C = representative(Cur.Index);
      unsigned Target = representative(Old.Index);
      while (C >= Target) {
        if (C == Target)
          return true;
        C = Values[C].Parent;
      }
      return false;
    }

    // private:
    /// Pick a representative for a sequence.
    unsigned representative(unsigned K) {
      if (Values[K].Merged)
        // Perform path compression as we go.
        return Values[K].Parent = representative(Values[K].Parent);
      return K;
    }
  };

  /// An object for which we can track unsequenced uses.
  using Object = Expr *;

  /// Different flavors of object usage which we track. We only track the
  /// least-sequenced usage of each kind.
  enum UsageKind {
    /// A read of an object. Multiple unsequenced reads are OK.
    UK_Use,

    /// A modification of an object which is sequenced before the value
    /// computation of the expression, such as ++n in C++.
    UK_ModAsValue,

    /// A modification of an object which is not sequenced before the value
    /// computation of the expression, such as n++.
    UK_ModAsSideEffect,

    UK_Count = UK_ModAsSideEffect + 1
  };

  struct Usage {
    Expr *Use;// = nullptr;
    SequenceTree::Seq Seq;

    Usage() : Use(nullptr)
    { }
  };

  struct UsageInfo {
    Usage Uses[UK_Count];

    /// Have we issued a diagnostic for this variable already?
    bool Diagnosed = false;

    UsageInfo() = default;
  };

  using UsageInfoMap = llvm::SmallDenseMap<Object, UsageInfo, 16>;

  clang::ASTContext *Context;

  /// Sequenced regions within the expression.
  SequenceTree Tree;

  /// Declaration modifications and references which we have seen.
  UsageInfoMap UsageMap;

  /// The region we are currently within.
  SequenceTree::Seq Region;

public:
  /// Map to store predicates related to undefined behaviour
  PREDICATE_MAP &predicateMap;

  typedef std::vector<Expr *> FN_CALL_LIST;
  llvm::DenseMap<Expr *, FN_CALL_LIST> exprFnCallMap;

private:
  Expr *topLevelExpr;

  llvm::DenseMap<Expr *, Expr *> parenMap;

  FN_CALL_LIST curFnCallList;

  /// Filled in with declarations which were modified as a side-effect
  /// (that is, post-increment operations).
  SmallVectorImpl<std::pair<Object, Usage>> *ModAsSideEffect = nullptr;

  /// Side effects which will remain even after an implicit seq point
  typedef std::vector<std::pair<Object, Usage>> GAMMA_LIST;

  GAMMA_LIST Gamma;

  /// Filled in with declarations which were modified as a value
  /// (that is, pre-increment operations).
  SmallVectorImpl<std::pair<Object, Usage>> *ModAsValue = nullptr;

  unsigned numSubseqEvalWithUnseqSE = 0;

  // To handle subexpressions where the VC of the result is seq wrt VC of the
  // subexpression
  // but unsequenced wrt SE of the subexpression
  struct SequencedSubexpressionWithUnseqSideEffects {
    SequencedSubexpressionWithUnseqSideEffects(
        SequenceChecker &Self, SequenceChecker::SequenceTree::Seq parentRegion)
        : Self(Self), parentRegion_(parentRegion),
          OldModAsSideEffect(Self.ModAsSideEffect),
          OldModAsValue(Self.ModAsValue) {
      Self.ModAsSideEffect = &ModAsSideEffect;
      Self.ModAsValue = &ModAsValue;
      ++Self.numSubseqEvalWithUnseqSE;
    }

    ~SequencedSubexpressionWithUnseqSideEffects() {
      for (auto MI = ModAsSideEffect.rbegin(), ME = ModAsSideEffect.rend();
           MI != ME; ++MI) {
        UsageInfo &U = Self.UsageMap[MI->first];
        auto &SideEffectUsage = U.Uses[UK_ModAsSideEffect];
        Self.replaceUsage(U, MI->first, SideEffectUsage.Use, UK_ModAsSideEffect,
                          parentRegion_);
      }

      --Self.numSubseqEvalWithUnseqSE;

      // Check whether the SE need to be carried upward or not
      if (Self.numSubseqEvalWithUnseqSE) {
        for (auto v : *(Self.ModAsSideEffect))
          OldModAsSideEffect->push_back(v);
      }

      Self.ModAsSideEffect = OldModAsSideEffect;
      Self.ModAsValue = OldModAsValue;
    }

    SequenceChecker &Self;
    SequenceChecker::SequenceTree::Seq parentRegion_;

    SmallVector<std::pair<Object, Usage>, 4> ModAsSideEffect;
    SmallVectorImpl<std::pair<Object, Usage>> *OldModAsSideEffect;

    SmallVector<std::pair<Object, Usage>, 4> ModAsValue;
    SmallVectorImpl<std::pair<Object, Usage>> *OldModAsValue;
  };

  /// Find the object which is produced by the specified expression, if any.
  Object getObject(Expr *E, bool Mod) const {
    E = E->IgnoreParenCasts();
    if (UnaryOperator *UO = dyn_cast<UnaryOperator>(E)) {
      if (Mod &&
          (UO->getOpcode() == UO_PostInc || UO->getOpcode() == UO_PostDec)) {
        return UO->getSubExpr()->IgnoreParenCasts();
      }
      if (Mod &&
          (UO->getOpcode() == UO_PreInc || UO->getOpcode() == UO_PreDec)) {
        return UO->getSubExpr()->IgnoreParenCasts();
      }
      if (UO->getOpcode() == UO_Deref) {
        return E;
      }
    } else if (BinaryOperator *BO = dyn_cast<BinaryOperator>(E)) {
      if (BO->getOpcode() == BO_Comma) {
        return getObject(BO->getRHS(), Mod);
      }
      if (Mod && BO->isAssignmentOp()) {
        return getObject(BO->getLHS(), Mod);
      }
    } else if (isa<ArraySubscriptExpr>(E) || isa<MemberExpr>(E) ||
               isa<DeclRefExpr>(E)) {
      return E;
    }
    return nullptr;
  }

  LOCN_TYPE getLocation(Object O) { return topLevelExpr; }

  Expr *getLCA(Expr *E1, Expr *E2) {
    std::unordered_set<Expr *> path1;

    Expr *traverse1 = E1;
    path1.insert(traverse1);
    while (parenMap.find(traverse1) != parenMap.end()) {
      traverse1 = parenMap[traverse1];
      path1.insert(traverse1);
    }

    Expr *traverse2 = E2;
    while (path1.find(traverse2) == path1.end()) {
      if (parenMap.find(traverse2) != parenMap.end())
        traverse2 = parenMap[traverse2];
      else
        // llvm_unreachable("LCA not found");
        return topLevelExpr;
    }

    return traverse2;
  }

  Expr *checkUnseq(Usage *Uses1, Usage *Uses2, UsageKind USE_TYPE_1,
                   UsageKind USE_TYPE_2) {
    if (Uses1[USE_TYPE_1].Use && Uses2[USE_TYPE_2].Use &&
        (Tree.isUnsequenced(Uses1[USE_TYPE_1].Seq, Region) ||
         Tree.isUnsequenced(Region, Uses1[USE_TYPE_1].Seq)))
      return getLCA(Uses1[USE_TYPE_1].Use->IgnoreParenCasts(),
                    Uses2[USE_TYPE_2].Use->IgnoreParenCasts());
    else
      return nullptr;
  }

  bool presentInGamma(Object O, Usage &U) {
    for (unsigned i = 0; i < Gamma.size(); i++) {
      if (Gamma[i].first == O && Gamma[i].second.Use == U.Use && 
            Gamma[i].second.Seq.getIndex() == U.Seq.getIndex())
        return true;
    }

    return false;
  }

  Expr *checkUnsequencedUsages(Usage *Uses1, Usage *Uses2, Object Obj1, Object Obj2, bool isAssignmentLikeOp = false) {
    if (Expr *retVal = checkUnseq(Uses1, Uses2, UK_Use, UK_ModAsValue))
      return retVal;
    else if (Expr *retVal =
                 checkUnseq(Uses1, Uses2, UK_Use, UK_ModAsSideEffect))
      return retVal;
    else if (Expr *retVal = checkUnseq(Uses1, Uses2, UK_ModAsValue, UK_Use))
      return retVal;
    else if (Expr *retVal =
                 checkUnseq(Uses1, Uses2, UK_ModAsValue, UK_ModAsValue))
      return retVal;
    else if (Expr *retVal =
                 checkUnseq(Uses1, Uses2, UK_ModAsValue, UK_ModAsSideEffect))
      return retVal;
    else if (Expr *retVal =
                 checkUnseq(Uses1, Uses2, UK_ModAsSideEffect, UK_Use))
      return retVal;
    else if (Expr *retVal =
                 checkUnseq(Uses1, Uses2, UK_ModAsSideEffect, UK_ModAsValue))
      return retVal;
    else if (Expr *retVal = checkUnseq(Uses1, Uses2, UK_ModAsSideEffect,
                                       UK_ModAsSideEffect)) {
      if (!isAssignmentLikeOp || (isAssignmentLikeOp && presentInGamma(Obj1, Uses1[UK_ModAsSideEffect])))
        return retVal;
      else {
        llvm::errs() << "Not Present in Gamma: " << Obj1 << "\n";
        return nullptr;
      }
    }
    else
      return nullptr;
  }

  bool checkTrivial(PREDICATE &pred) {
    // Check if both the exprs are different decl refs, in which case,
    // skip adding as that is a trivial predicate (in the C language)
    return !Context->getLangOpts().CPlusPlus && isa<DeclRefExpr>(pred[0]) &&
           isa<DeclRefExpr>(pred[1])
           // Note : Used a simple pointer check here
           && (pred[0] != pred[1]);
  }

  void addToPredicateMap(UsageInfo &UI, Object O, Expr *Ref, bool isAssignmentLikeOp = false) {
    for (auto it1 = UsageMap.begin(); it1 != UsageMap.end(); it1++) {
      if (it1->first == O) {
        continue;
      }

      auto &Uses1 = it1->second.Uses;
      auto &Uses2 = UI.Uses;
      PREDICATE pred = {O, it1->first};

      if (Expr *locVal = checkUnsequencedUsages(Uses1, Uses2, it1->first, O, isAssignmentLikeOp)) {
        if (!checkTrivial(pred)) {
          // Note : Check the region in which the addition needs to be performed
          // Add to predicate map

          LOCN_TYPE loc = topLevelExpr;

          // Insert the list of function call which the predicate depends on
          // This needs to be done after the traversal is over as only then the
          // exprToFnCall map is guaranteed to be fully populated
          // Thus we store the LCA currently and later use it to populate the fn
          // call list
          pred.push_back(locVal);

          if (O < it1->first) {
            predicateMap[loc].insert(pred);
          } else {
            std::swap(pred[0], pred[1]);
            predicateMap[loc].insert(pred);
          }
        }
      }
    }
  }

  /// Note that an object was modified or used by an expression.
  void addUsage(UsageInfo &UI, Object O, Expr *Ref, UsageKind UK, bool isAssignmentLikeOp = false) {
    Usage &U = UI.Uses[UK];

    if (!U.Use || (!Tree.isUnsequenced(Region, U.Seq) &&
                   !Tree.isUnsequenced(U.Seq, Region))) {
      if (UK == UK_ModAsSideEffect && ModAsSideEffect)
          ModAsSideEffect->push_back(std::make_pair(O, U));

      U.Use = Ref;
      U.Seq = Region;

      if (UK == UK_ModAsSideEffect)
        Gamma.push_back(std::make_pair(O, U));
    }
    
    addToPredicateMap(UI, O, Ref, isAssignmentLikeOp);
  }

  void replaceUsage(UsageInfo &UI, Object O, Expr *Ref, UsageKind UK,
                    SequenceTree::Seq addRegion) {
    Usage &U = UI.Uses[UK];
    U.Use = Ref;
    U.Seq = addRegion;

    if (UK == UK_ModAsSideEffect) {
      GAMMA_LIST replacementGamma;

      for (unsigned i = 0; i < Gamma.size(); i++) {
        if (Gamma[i].first == O) {
          replacementGamma.push_back(std::make_pair(O, U));
        } else {
          replacementGamma.push_back(Gamma[i]);
        }
      }

      Gamma = replacementGamma;
    }
  }

  void notePreUse(Object O, Expr *Use) {}
  void notePreMod(Object O, Expr *Mod) {}

  void notePostUse(Object O, Expr *Use) {
    // Check that O is a scalar object
    if (!O || !O->isLValue())
      return;

    if (const Type *type = O->getType().getTypePtrOrNull()) {
      if (type->isScalarType()) {
        addUsage(UsageMap[O], O, Use, UK_Use);
      }
    }
  }

  void notePostMod(Object O, Expr *Use, UsageKind UK, bool isAssignmentLikeOp = false) {
    // Check that O is a scalar object
    if (!O || !O->isLValue())
      return;

    if (const Type *type = O->getType().getTypePtrOrNull()) {
      if (type->isScalarType()) {
        addUsage(UsageMap[O], O, Use, UK, isAssignmentLikeOp);
      }
    }
  }

  void insertInCallList(CallExpr *CE) { curFnCallList.push_back(CE); }

  void backupFnCallList(FN_CALL_LIST &oldList) {
    oldList = curFnCallList;
    curFnCallList.clear();
  }

  void mergeAndRestoreFnCallList(Expr *node, FN_CALL_LIST &oldList) {
    exprFnCallMap[node] = curFnCallList;
    curFnCallList.insert(curFnCallList.end(), oldList.begin(), oldList.end());
  }

  void backupGamma(GAMMA_LIST &oldGamma) {
    oldGamma = Gamma;
    Gamma.clear();
  }

  void mergeAndRestoreGamma(GAMMA_LIST &oldGamma) {
    Gamma.insert(Gamma.end(), oldGamma.begin(), oldGamma.end());
    // printGamma();
  }

public:
  SequenceChecker(clang::ASTContext *ctx, Expr *E, PREDICATE_MAP &predicateMap,
                  Expr *topLevelExpr)
      : Base(*ctx), Context(ctx), Region(Tree.root()),
        predicateMap(predicateMap), topLevelExpr(topLevelExpr) {
    Visit(E);
  }

  /*
   * The invariants of all the visit functions will essentially be:
   * - Visiting the subexpressions
   * - Note their use or modifications as suitable
   * - Generate predicates as per the sequencing rules
   * - Pass the usage information towards the root i.e upwards
   */

  void VisitStmt(Stmt *S) {
    // Skip all statements which aren't expressions for now.
  }

  void VisitWhileNotingUse(Expr *E) {
    // Record the use of the subexpressions
    Object O = getObject(E, false);

    SequenceTree::Seq OldRegion = Region;

    SequenceTree::Seq subexpr_eval_region = Tree.allocate(OldRegion);
    SequenceTree::Seq obj_use_region = Tree.allocate(OldRegion);

    notePreUse(O, E);

    {
      SequencedSubexpressionWithUnseqSideEffects subexprEvaluator(*this,
                                                                  OldRegion);

      // The evaluation of the Sub Expression along with the side effects
      Region = subexpr_eval_region;
      Visit(E);
    }

    Region = obj_use_region;
    notePostUse(O, E);

    // Merge the other regions
    Tree.merge(subexpr_eval_region);
    Tree.merge(obj_use_region);

    // Making all the other VC and SE unsequenced with the previous ones
    Region = OldRegion;
  }

  void VisitExpr(Expr *E) {
    // By default, just recurse to evaluated subexpressions.
    printUsageMap("In VISIT EXPRESSION - USAGE MAP - PRE RECURSION");

    FN_CALL_LIST oldList;
    backupFnCallList(oldList);

    GAMMA_LIST oldGamma;
    backupGamma(oldGamma);

    for (auto it = E->child_begin(); it != E->child_end(); ++it) {
      if (!*it)
        continue;

      // Record the use of the subexpressions
      if (Expr *e = dyn_cast<Expr>(*it)) {
        if (e->IgnoreParenCasts() != E->IgnoreParenCasts())
          parenMap[e->IgnoreParenCasts()] = E->IgnoreParenCasts();
        VisitWhileNotingUse(e);
      }
    }

    mergeAndRestoreFnCallList(E->IgnoreParenCasts(), oldList);
    mergeAndRestoreGamma(oldGamma);

    printUsageMap("In VISIT EXPRESSION - USAGE MAP - POST RECURSION");
  }

  void VisitParenExpr(ParenExpr *PE) { Visit(PE->getSubExpr()); }

  void VisitGenericSelectionExpr(GenericSelectionExpr *E) {
    // The controlling expression of a generic selection is not evaluated.

    Base::VisitGenericSelectionExpr(E);
  }

  void VisitUnaryExprOrTypeTraitExpr(UnaryExprOrTypeTraitExpr *E) {
    // sizeof and _Alignof not visited unless VLAs

    // FIXME: Add support for VLAs
    Base::VisitUnaryExprOrTypeTraitExpr(E);
  }

  void VisitBinComma(BinaryOperator *BO) {
    // C++11 [expr.comma]p1:
    //   Every value computation and side effect associated with the left
    //   expression is sequenced before every value computation and side
    //   effect associated with the right expression.
    SequenceTree::Seq LHS = Tree.allocate(Region);
    SequenceTree::Seq RHS = Tree.allocate(Region);
    SequenceTree::Seq OldRegion = Region;

    {
      FN_CALL_LIST oldList;
      backupFnCallList(oldList);

      GAMMA_LIST oldGamma;
      backupGamma(oldGamma);

      // Visit LHS and RHS in different regions but do not consider alias
      // predicates
      // between them as they are sequenced. To achieve this, we bubble up the
      // side
      // effects only after both have been visited, unlike assignment, etc.
      // where
      // the side effects of LHS are made available early, so that they can
      // allow the
      // generation of predicates with uses/side-effects of the RHS
      SequencedSubexpressionWithUnseqSideEffects subexprEvaluator(*this,
                                                                  OldRegion);
      Region = LHS;

      printUsageMap("In VISIT BIN COMMA EXPRESSION - USAGE MAP - PRE LHS");
      parenMap[BO->getLHS()->IgnoreParenCasts()] = BO->IgnoreParenCasts();
      Visit(BO->getLHS());

      Gamma.clear();

      printUsageMap(
          "In VISIT BIN COMMA EXPRESSION - USAGE MAP - POST LHS - PRE RHS");
      Region = RHS;

      parenMap[BO->getRHS()->IgnoreParenCasts()] = BO->IgnoreParenCasts();
      Visit(BO->getRHS());
      printUsageMap("In VISIT BIN COMMA EXPRESSION - USAGE MAP - POST RHS");

      mergeAndRestoreFnCallList(BO->IgnoreParenCasts(), oldList);
      mergeAndRestoreGamma(oldGamma);
    }

    Region = OldRegion;

    // LHS and RHS are both unsequenced with respect to other stuff.
    Tree.merge(LHS);
    Tree.merge(RHS);
  }

  void VisitBinAssign(BinaryOperator *BO) {
    // The modification is sequenced after the value computation of the LHS
    // and RHS, so check it before inspecting the operands and update the
    // map afterwards.
    Object O = getObject(BO->getLHS(), true);
    if (!O)
      return VisitExpr(BO);

    printUsageMap("In VISIT BIN ASSIGN EXPRESSION - USAGE MAP - PRE LHS");

    notePreMod(O, BO);

    SequenceTree::Seq OldRegion = Region;

    SequenceTree::Seq subexpr_eval_region = Tree.allocate(OldRegion);
    SequenceTree::Seq obj_mod_region = Tree.allocate(OldRegion);

    {
      FN_CALL_LIST oldList;
      backupFnCallList(oldList);

      GAMMA_LIST oldGamma;
      backupGamma(oldGamma);

      SequencedSubexpressionWithUnseqSideEffects subexprEvaluator(*this,
                                                                  OldRegion);

      // The evaluation of the Sub Expressions along with the side effects
      Region = subexpr_eval_region;
      // C++11 [expr.ass]p7:
      //   E1 op= E2 is equivalent to E1 = E1 op E2, except that E1 is evaluated
      //   only once.
      //
      // Therefore, for a compound assignment operator, O is considered used
      // everywhere except within the evaluation of E1 itself.
      if (isa<CompoundAssignOperator>(BO))
        notePreUse(O, BO);

      parenMap[BO->getLHS()->IgnoreParenCasts()] = BO->IgnoreParenCasts();
      Visit(BO->getLHS());

      if (isa<CompoundAssignOperator>(BO))
        notePostUse(O, BO);

      printUsageMap(
          "In VISIT BIN ASSIGN EXPRESSION - USAGE MAP - POST LHS - PRE RHS");

      parenMap[BO->getRHS()->IgnoreParenCasts()] = BO->IgnoreParenCasts();
      Visit(BO->getRHS());

      mergeAndRestoreFnCallList(BO->IgnoreParenCasts(), oldList);
      mergeAndRestoreGamma(oldGamma);
    }

    // Noting the modification of the subexpr object as sequenced with the value
    // computation
    Region = obj_mod_region;

    // C++11 [expr.ass]p1:
    //   the assignment is sequenced [...] before the value computation of the
    //   assignment expression.
    // C11 6.5.16/3 has no such rule.
    notePostMod(O, BO, Context->getLangOpts().CPlusPlus ? UK_ModAsValue
                                                        : UK_ModAsSideEffect, true);

    printUsageMap("In VISIT BIN ASSIGN EXPRESSION - USAGE MAP - POST RHS");

    // Merge the sub regions back
    Tree.merge(subexpr_eval_region);
    Tree.merge(obj_mod_region);

    Region = OldRegion;
  }

  void VisitCompoundAssignOperator(CompoundAssignOperator *CAO) {
    VisitBinAssign(CAO);
  }

  void VisitUnaryPreInc(UnaryOperator *UO) { VisitUnaryPreIncDec(UO); }
  void VisitUnaryPreDec(UnaryOperator *UO) { VisitUnaryPreIncDec(UO); }
  void VisitUnaryPreIncDec(UnaryOperator *UO) {
    Object O = getObject(UO->getSubExpr(), true);

    if (!O) {
      FN_CALL_LIST oldList;
      backupFnCallList(oldList);

      GAMMA_LIST oldGamma;
      backupGamma(oldGamma);

      parenMap[UO->getSubExpr()->IgnoreParenCasts()] = UO->IgnoreParenCasts();
      VisitExpr(UO->getSubExpr());

      mergeAndRestoreFnCallList(UO->IgnoreParenCasts(), oldList);
      mergeAndRestoreGamma(oldGamma);
      return;
    }

    printUsageMap(
        "In VISIT UNARY PRE INC DEC EXPRESSION - USAGE MAP - PRE EXPRESSION");

    SequenceTree::Seq OldRegion = Region;

    SequenceTree::Seq subexpr_eval_region = Tree.allocate(OldRegion);
    SequenceTree::Seq obj_mod_region = Tree.allocate(OldRegion);

    notePreMod(O, UO->getSubExpr());

    {
      FN_CALL_LIST oldList;
      backupFnCallList(oldList);

      GAMMA_LIST oldGamma;
      backupGamma(oldGamma);

      SequencedSubexpressionWithUnseqSideEffects subexprEvaluator(*this,
                                                                  OldRegion);

      // The evaluation of the Sub Expression along with the side effects
      Region = subexpr_eval_region;
      parenMap[UO->getSubExpr()->IgnoreParenCasts()] = UO->IgnoreParenCasts();
      Visit(UO->getSubExpr());

      mergeAndRestoreFnCallList(UO->IgnoreParenCasts(), oldList);
      mergeAndRestoreGamma(oldGamma);
    }

    // Noting the modification of the subexpr object as sequenced with the value
    // computation
    Region = obj_mod_region;
    // C++11 [expr.pre.incr]p1:
    //   the expression ++x is equivalent to x+=1

    // Note : This is even true in the C standard(6.5.3.1). However, the
    // treatment of += is
    // different in both C and C++, as has earlier been pointed out.
    notePostMod(O, UO->getSubExpr(), Context->getLangOpts().CPlusPlus
                                         ? UK_ModAsValue
                                         : UK_ModAsSideEffect, true);

    printUsageMap(
        "In VISIT UNARY PRE INC DEC EXPRESSION - USAGE MAP - POST EXPRESSION");

    // Merge the other regions
    Tree.merge(subexpr_eval_region);
    Tree.merge(obj_mod_region);

    // Making all the other VC and SE unsequenced with the previous ones
    Region = OldRegion;
  }

  void VisitUnaryPostInc(UnaryOperator *UO) { VisitUnaryPostIncDec(UO); }
  void VisitUnaryPostDec(UnaryOperator *UO) { VisitUnaryPostIncDec(UO); }
  void VisitUnaryPostIncDec(UnaryOperator *UO) {
    Object O = getObject(UO->getSubExpr(), true);

    if (!O) {
      FN_CALL_LIST oldList;
      backupFnCallList(oldList);

      GAMMA_LIST oldGamma;
      backupGamma(oldGamma);

      parenMap[UO->getSubExpr()->IgnoreParenCasts()] = UO->IgnoreParenCasts();
      VisitExpr(UO->getSubExpr());

      mergeAndRestoreFnCallList(UO->IgnoreParenCasts(), oldList);
      mergeAndRestoreGamma(oldGamma);
      return;
    }

    printUsageMap(
        "In VISIT UNARY PRE INC DEC EXPRESSION - USAGE MAP - PRE EXPRESSION");

    SequenceTree::Seq OldRegion = Region;

    SequenceTree::Seq subexpr_eval_region = Tree.allocate(OldRegion);

    notePreMod(O, UO->getSubExpr());

    {
      FN_CALL_LIST oldList;
      backupFnCallList(oldList);

      GAMMA_LIST oldGamma;
      backupGamma(oldGamma);

      SequencedSubexpressionWithUnseqSideEffects subexprEvaluator(*this,
                                                                  OldRegion);

      // The evaluation of the Sub Expression along with the side effects
      Region = subexpr_eval_region;
      parenMap[UO->getSubExpr()->IgnoreParenCasts()] = UO->IgnoreParenCasts();
      Visit(UO->getSubExpr());

      mergeAndRestoreFnCallList(UO->IgnoreParenCasts(), oldList);
      mergeAndRestoreGamma(oldGamma);
    }

    // Making the mod as SE along with subsequent VC and SE unsequenced with the
    // previous ones
    SequenceTree::Seq final_region = Tree.allocate(OldRegion);
    Region = final_region;

    notePostMod(O, UO->getSubExpr(), UK_ModAsSideEffect, true);

    printUsageMap(
        "In VISIT UNARY PRE INC DEC EXPRESSION - USAGE MAP - POST EXPRESSION");

    // Merge the regions
    Tree.merge(subexpr_eval_region);
    Tree.merge(final_region);

    Region = OldRegion;
  }

  void VisitUnaryDeref(UnaryOperator *UO) {
    FN_CALL_LIST oldList;
    backupFnCallList(oldList);

    GAMMA_LIST oldGamma;
    backupGamma(oldGamma);

    parenMap[UO->getSubExpr()->IgnoreParenCasts()] = UO->IgnoreParenCasts();
    VisitWhileNotingUse(UO->getSubExpr());

    mergeAndRestoreFnCallList(UO->IgnoreParenCasts(), oldList);
    mergeAndRestoreGamma(oldGamma);
  }

  void VisitUnaryAddrOf(UnaryOperator *UO) {
    Expr *E = UO->getSubExpr();
    E = E->IgnoreParenCasts();
    // Address conditions of &* and &[] as given in section 6.5.3.2 of the C11
    // Standard
    if (UnaryOperator *UO_Sub = dyn_cast<UnaryOperator>(E)) {
      if (UO_Sub->getOpcode() == UO_Deref) {
        FN_CALL_LIST oldList;
        backupFnCallList(oldList);

        GAMMA_LIST oldGamma;
        backupGamma(oldGamma);

        parenMap[UO_Sub->getSubExpr()->IgnoreParenCasts()] =
            UO->IgnoreParenCasts();
        VisitWhileNotingUse(UO_Sub->getSubExpr());

        mergeAndRestoreFnCallList(UO->IgnoreParenCasts(), oldList);
        mergeAndRestoreGamma(oldGamma);
        return;
      }
    }

    if (ArraySubscriptExpr *ASE = dyn_cast<ArraySubscriptExpr>(E)) {
      FN_CALL_LIST oldList;
      backupFnCallList(oldList);

      GAMMA_LIST oldGamma;
      backupGamma(oldGamma);

      parenMap[ASE->getLHS()->IgnoreParenCasts()] =
          parenMap[ASE->getRHS()->IgnoreParenCasts()] = UO->IgnoreParenCasts();
      VisitWhileNotingUse(ASE->getLHS());
      VisitWhileNotingUse(ASE->getRHS());

      mergeAndRestoreFnCallList(UO->IgnoreParenCasts(), oldList);
      mergeAndRestoreGamma(oldGamma);
      return;
    }

    FN_CALL_LIST oldList;
    backupFnCallList(oldList);

    GAMMA_LIST oldGamma;
    backupGamma(oldGamma);

    // Need not visit this operator explicitly as it will be a temporary object
    parenMap[UO->getSubExpr()->IgnoreParenCasts()] = UO->IgnoreParenCasts();
    VisitWhileNotingUse(UO->getSubExpr());

    mergeAndRestoreFnCallList(UO->IgnoreParenCasts(), oldList);
    mergeAndRestoreGamma(oldGamma);
  }

  void VisitArraySubscriptExpr(ArraySubscriptExpr *ASE) {
    FN_CALL_LIST oldList;
    backupFnCallList(oldList);

    GAMMA_LIST oldGamma;
    backupGamma(oldGamma);

    parenMap[ASE->getLHS()->IgnoreParenCasts()] =
        parenMap[ASE->getRHS()->IgnoreParenCasts()] = ASE->IgnoreParenCasts();
    VisitWhileNotingUse(ASE->getLHS());
    VisitWhileNotingUse(ASE->getRHS());

    mergeAndRestoreFnCallList(ASE->IgnoreParenCasts(), oldList);
    mergeAndRestoreGamma(oldGamma);
  }

  void VisitMemberExpr(MemberExpr *ME) {
    FN_CALL_LIST oldList;
    backupFnCallList(oldList);

    GAMMA_LIST oldGamma;
    backupGamma(oldGamma);

    parenMap[ME->getBase()->IgnoreParenCasts()] = ME->IgnoreParenCasts();
    VisitWhileNotingUse(ME->getBase());

    mergeAndRestoreFnCallList(ME->IgnoreParenCasts(), oldList);
    mergeAndRestoreGamma(oldGamma);
  }

  bool evaluate(const Expr *E, bool &result) {
    bool isConst = false;
    if (!E->isValueDependent())
      isConst = E->EvaluateAsBooleanCondition(result, *(Context));
    return isConst;
  }

  /// Don't visit the RHS of '&&' or '||' if it might not be evaluated.
  void VisitBinLOr(BinaryOperator *BO) {
    SequenceTree::Seq LHS = Tree.allocate(Region);
    SequenceTree::Seq RHS = Tree.allocate(Region);
    SequenceTree::Seq OldRegion = Region;

    {
      FN_CALL_LIST oldList;
      backupFnCallList(oldList);

      GAMMA_LIST oldGamma;
      backupGamma(oldGamma);

      // Don't visit the RHS of '&&' or '||' if it might not be evaluated.
      // However, it may be impossible to tell at compile time, so don't
      // visit RHS and note the usages only from the LHS
      SequencedSubexpressionWithUnseqSideEffects subexprEvaluator(*this,
                                                                  OldRegion);

      Region = LHS;
      parenMap[BO->getLHS()->IgnoreParenCasts()] = BO->IgnoreParenCasts();
      Visit(BO->getLHS());

      Gamma.clear();

      // If LHS is statically false, visit the RHS as well
      bool LHSResult;
      if (evaluate(BO->getLHS(), LHSResult) && !LHSResult) {
        Region = RHS;
        parenMap[BO->getRHS()->IgnoreParenCasts()] = BO->IgnoreParenCasts();
        Visit(BO->getRHS());
      }

      mergeAndRestoreFnCallList(BO->IgnoreParenCasts(), oldList);
      mergeAndRestoreGamma(oldGamma);
    }

    Region = OldRegion;

    // LHS and RHS are both unsequenced with respect to other stuff.
    Tree.merge(LHS);
    Tree.merge(RHS);
  }

  void VisitBinLAnd(BinaryOperator *BO) {
    // VisitWhileNotingUse(BO->getLHS());
    SequenceTree::Seq LHS = Tree.allocate(Region);
    SequenceTree::Seq RHS = Tree.allocate(Region);
    SequenceTree::Seq OldRegion = Region;

    {
      FN_CALL_LIST oldList;
      backupFnCallList(oldList);

      GAMMA_LIST oldGamma;
      backupGamma(oldGamma);

      // Don't visit the RHS of '&&' or '||' if it might not be evaluated.
      // However, it may be impossible to tell at compile time, so don't
      // visit RHS and note the usages only from the LHS
      SequencedSubexpressionWithUnseqSideEffects subexprEvaluator(*this,
                                                                  OldRegion);

      Region = LHS;
      parenMap[BO->getLHS()->IgnoreParenCasts()] = BO->IgnoreParenCasts();
      Visit(BO->getLHS());

      Gamma.clear();

      // If LHS is statically true, visit the RHS as well
      bool LHSResult;
      if (evaluate(BO->getLHS(), LHSResult) && LHSResult) {
        Region = RHS;
        parenMap[BO->getRHS()->IgnoreParenCasts()] = BO->IgnoreParenCasts();
        Visit(BO->getRHS());
      }

      mergeAndRestoreFnCallList(BO->IgnoreParenCasts(), oldList);
      mergeAndRestoreGamma(oldGamma);
    }

    Region = OldRegion;

    // LHS and RHS are both unsequenced with respect to other stuff.
    Tree.merge(LHS);
    Tree.merge(RHS);
  }

  void VisitAbstractConditionalOperator(AbstractConditionalOperator *CO) {
    // VisitWhileNotingUse(CO->getCond());
    SequenceTree::Seq Condition = Tree.allocate(Region);
    SequenceTree::Seq Branch = Tree.allocate(Region);
    SequenceTree::Seq OldRegion = Region;

    {
      FN_CALL_LIST oldList;
      backupFnCallList(oldList);

      GAMMA_LIST oldGamma;
      backupGamma(oldGamma);

      // Only visit the condition, the subexpressions can cause predicates
      // but may never be evaluated at run-time, so don't visit it
      SequencedSubexpressionWithUnseqSideEffects subexprEvaluator(*this,
                                                                  OldRegion);

      Region = Condition;
      parenMap[CO->getCond()->IgnoreParenCasts()] = CO->IgnoreParenCasts();
      Visit(CO->getCond());

      Gamma.clear();

      // If condition is statically true/false, visit the branches as well
      bool cond;
      if (evaluate(CO->getCond(), cond)) {
        Region = Branch;
        if (cond) {
          parenMap[CO->getTrueExpr()->IgnoreParenCasts()] =
              CO->IgnoreParenCasts();
          Visit(CO->getTrueExpr());
        } else {
          parenMap[CO->getFalseExpr()->IgnoreParenCasts()] =
              CO->IgnoreParenCasts();
          Visit(CO->getFalseExpr());
        }
      }

      mergeAndRestoreFnCallList(CO->IgnoreParenCasts(), oldList);
      mergeAndRestoreGamma(oldGamma);
    }

    Region = OldRegion;

    // Condition and Branch are both unsequenced with respect to other stuff.
    Tree.merge(Condition);
    Tree.merge(Branch);
  }

  void VisitCallExpr(CallExpr *CE) {
    // C++11 [intro.execution]p15:
    //   When calling a function [...], every value computation and side effect
    //   associated with any argument expression, or with the postfix expression
    //   designating the called function, is sequenced before execution of every
    //   expression or statement in the body of the function [and thus before
    //   the value computation of its result].
    
    GAMMA_LIST oldGamma;
    backupGamma(oldGamma);

    VisitExpr(CE);
    insertInCallList(CE);

    Gamma.clear();

    mergeAndRestoreGamma(oldGamma);
    // FIXME: Check for sequencing in functions with ellipses like printf etc.
    // FIXME: CXXNewExpr and CXXDeleteExpr implicitly call functions.
  }

  void VisitInitListExpr(InitListExpr *ILE) {
    // In C++11, list initializations are sequenced.

    // In C11, the initialiser list evaluation is indeterminately seq and hence
    // in this analysis for unsequenced races, we will assume it to be sequenced
    // like in C++ (Section 6.7.9 Point 23)
    SmallVector<SequenceTree::Seq, 32> Elts;
    SequenceTree::Seq Parent = Region;

    FN_CALL_LIST oldList;
    backupFnCallList(oldList);

    GAMMA_LIST oldGamma;
    backupGamma(oldGamma);

    for (unsigned I = 0; I < ILE->getNumInits(); ++I) {
      Expr *E = ILE->getInit(I);
      if (!E)
        continue;
      Region = Tree.allocate(Parent);
      Elts.push_back(Region);

      // Record the use of the init list expressions
      parenMap[E->IgnoreParenCasts()] = ILE->IgnoreParenCasts();
      VisitWhileNotingUse(E);

      Gamma.clear();
    }

    mergeAndRestoreFnCallList(ILE->IgnoreParenCasts(), oldList);
    mergeAndRestoreGamma(oldGamma);

    // Forget that the initializers are sequenced.
    Region = Parent;
    for (unsigned I = 0; I < Elts.size(); ++I)
      Tree.merge(Elts[I]);
  }

  void printUsageMap(std::string location = "") {
#ifdef LOG_DEBUG_STATEMENTS
    llvm::errs() << "----------------------------------------------------------"
                    "------------\n";
    llvm::errs() << location << "\n";

    for (auto &o : UsageMap) {
      auto &Uses = o.second.Uses;
      for (int i = 0; i < UK_Count; i++) {
        if (Uses[i].Use) {
          auto loc = Uses[i].Use->getExprLoc();
          int id = Context->getDiagnostics().getCustomDiagID(
              DiagnosticsEngine::Level::Remark,
              "UsageType : %0 Object : %1 Region : %2 Expr : ");
          DiagnosticBuilder DB = Context->getDiagnostics().Report(loc, id);
          DB << i << o.first << Uses[i].Seq.getIndex() << SourceRange(loc);
        }
      }
    }
    llvm::errs() << "----------------------------------------------------------"
                    "------------\n\n";
#endif
  }

  void printGamma(std::string location = "") {
    llvm::errs() << "----------------------------------------------------------"
                    "------------\n";
    llvm::errs() << location << "\n";

    for (auto &o : Gamma) {
      if (o.second.Use) {
        auto loc = o.second.Use->getExprLoc();
        int id = Context->getDiagnostics().getCustomDiagID(
            DiagnosticsEngine::Level::Remark,
            "Object : %0 Region : %1 Expr : ");
        DiagnosticBuilder DB = Context->getDiagnostics().Report(loc, id);
        DB << o.first << o.second.Seq.getIndex() << SourceRange(loc);
      } else {
        llvm::errs() << "USE IS EMPTY!!!\n";
      }
    }
    llvm::errs() << "----------------------------------------------------------"
                    "------------\n\n";
  }
};

bool SeqVisitor::processExpr(Expr *E) {
  SequenceChecker sc(Context, E, *predicateMap, E);
  if (predicateMap->count(E)) {
    FULL_EXPRS++;
  }

  // Add the fn call info to the predicates
  auto &predMap = sc.predicateMap;
  auto &exprFnCallMap = sc.exprFnCallMap;

  std::set<PREDICATE> predSet;
  for (auto set_it = predMap[E].begin(); set_it != predMap[E].end(); set_it++) {
    PREDICATE tempPred = *set_it;
    Expr *lca = tempPred.back();
    tempPred.pop_back();
    if (exprFnCallMap.find(lca) != exprFnCallMap.end()) {
      auto &fnCallList = exprFnCallMap[lca];
      tempPred.insert(tempPred.end(), fnCallList.begin(), fnCallList.end());

      if (fnCallList.size())
        NUM_FUNC_PREDS++;
    } else {
      llvm::errs() << "LCA not found in expr fn call map\n";
    }
    NUM_PREDICATES++;
    predSet.insert(tempPred);
  }
  predMap[E] = predSet;
  return true;
}

bool SeqVisitor::VisitStmt(Stmt *S) {
  if (S && dyn_cast<Expr>(S))
    return true;

  for (auto it = S->child_begin(); it != S->child_end(); ++it) {
    if (!*it)
      continue;

    if (Expr *Exp = dyn_cast<Expr>(*it))
      if (!processExpr(Exp))
        return false;
  }

  return true;
}

class UnsequencedAliasVisitorAction : public PluginASTAction {
protected:
  std::unique_ptr<ASTConsumer>
  CreateASTConsumer(CompilerInstance &CI, llvm::StringRef InFile) override {
    // Create predicate map and predicate output file name
    if (!CI.hasPredicateMap())
      CI.createPredicateMap();

    std::string predicateFileName =
        getFileNameFromPath(InFile.str()) + ".preds";

    // Enable Sanitizer options to emit __no_alias predicates
    CI.getLangOpts().Sanitize.set(SanitizerKind::UnsequencedScalars, true);
    CI.getCodeGenOpts().SanitizeRecover.set(SanitizerKind::UnsequencedScalars,
                                            true);

    return std::make_unique<SeqConsumer>(
        &CI.getASTContext(), CI.getPredicateMap(), predicateFileName,
        CI.getSourceManager());
  }

  bool ParseArgs(const CompilerInstance &CI,
                 const std::vector<std::string> &args) override {
    if (!args.empty() && args[0] == "help")
      PrintHelp(llvm::errs());

    return true;
  }

  void PrintHelp(llvm::raw_ostream &ros) {
    ros << "This plugin requires no arguments\n";
  }

  ActionType getActionType() override {
    return clang::PluginASTAction::AddBeforeMainAction;
  }
};
}

static FrontendPluginRegistry::Add<UnsequencedAliasVisitorAction>
    X("unseq", "unsequenced race analysis");
