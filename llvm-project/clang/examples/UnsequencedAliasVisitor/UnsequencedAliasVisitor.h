#ifndef UNSEQUENCED_ALIAS_VISITOR_H
#define UNSEQUENCED_ALIAS_VISITOR_H

#include "clang/AST/ASTConsumer.h"
#include "clang/AST/ASTContext.h"
#include "clang/AST/EvaluatedExprVisitor.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/Basic/CodeGenOptions.h"
#include "clang/Basic/Diagnostic.h"
#include "clang/Basic/DiagnosticOptions.h"
#include "clang/Basic/FileManager.h"
#include "clang/Basic/Sanitizers.h"
#include "clang/Basic/SourceManager.h"
#include "clang/Basic/TargetInfo.h"
#include "clang/Basic/TargetOptions.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Frontend/FrontendPluginRegistry.h"
#include "clang/Sema/SemaDiagnostic.h"

#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/ADT/Triple.h"
#include "llvm/Analysis/TargetLibraryInfo.h"
#include "llvm/Support/Host.h"
#include "llvm/Support/SourceMgr.h"
#include "llvm/Support/raw_ostream.h"

#include <set>
#include <string>

std::string getFileNameFromPath(std::string path);
std::string printExpr(const clang::Expr *E);
void printAliasRules(clang::PREDICATE_MAP &predicateMap, std::string &filename,
                     clang::SourceManager &SM);

namespace clang {

class SeqVisitor : public clang::RecursiveASTVisitor<SeqVisitor> {
public:
  clang::ASTContext *Context;
  PREDICATE_MAP *predicateMap;

  SeqVisitor() : Context(nullptr), predicateMap(nullptr){};
  SeqVisitor(clang::ASTContext *ctx, PREDICATE_MAP *pm)
      : Context(ctx), predicateMap(pm){};

  bool processExpr(clang::Expr *d);
  bool VisitStmt(clang::Stmt *s);
};

class SeqConsumer : public clang::ASTConsumer {

private:
  SeqVisitor visitor_;
  std::string &OutFileName_;
  clang::SourceManager &SourceManager_;

public:
  SeqConsumer(clang::ASTContext *ctx, PREDICATE_MAP *pm,
              std::string &OutFileName, clang::SourceManager &SM)
      : visitor_(SeqVisitor(ctx, pm)), OutFileName_(OutFileName),
        SourceManager_(SM){};

  virtual ~SeqConsumer() {}
  virtual bool HandleTopLevelDecl(clang::DeclGroupRef d);
  virtual void PrintStats();
};
}

#endif
