//------------------------------------------------------------------------------
// OpenMP to SPMD translator
// Using code(rewritersample.cpp) from Eli Bendersky (eliben@gmail.com) as starting point
// This code is in the public domain
//------------------------------------------------------------------------------
#ifndef OMPTOSPMD_H
#define OMPTOSPMD_H
#include <cstdio>
#include <string>
#include <sstream>
#include <string>
#include <vector>
#include <set>
#include <map>

#include "clang/AST/ASTConsumer.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/AST/OpenMPClause.h"
#include "clang/Basic/Diagnostic.h"
#include "clang/Basic/FileManager.h"
#include "clang/Basic/SourceManager.h"
#include "clang/Basic/TargetOptions.h"
#include "clang/Basic/TargetInfo.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Frontend/TextDiagnosticPrinter.h"
#include "clang/Lex/Preprocessor.h"
#include "clang/Parse/ParseAST.h"
#include "clang/Rewrite/Core/Rewriter.h"
#include "clang/Rewrite/Frontend/Rewriters.h"
#include "llvm/Support/Host.h"
#include "llvm/Support/raw_ostream.h"

using namespace clang;

// By implementing RecursiveASTVisitor, we can specify which AST nodes
// we're interested in by overriding relevant methods.
class MyASTVisitor : public RecursiveASTVisitor<MyASTVisitor> {
private:
  Rewriter &TheRewriter ;
  std::set<Stmt*> stmt_seen ;
  unsigned int reduction_counter ;
  unsigned int barrier_counter ;
  bool new_scope ;
  std::map<Stmt*,Stmt*> StmtAncestors ;
  std::set<Stmt*> barrierPresentAtScope ;
  std::vector<Stmt*> StmtStack ;
  FunctionDecl* curFunctionDecl ;

  void removePragma(Stmt* s) ;

  void insertStartEndIndxInFor(Stmt* s) ;

  //KCFIX : This is very dumb
  void insertBarrier(Stmt* s) ;

  //KCFIX : Has to be rewritten to reconstruct the prototype rather than inserting
  void insertStartEndIndxInFn(FunctionDecl *f) ;

  //KCFIX : Has to be rewritten to reconstruct the prototype rather than inserting
  void handleReduction(Stmt* s) ;

public:
  MyASTVisitor(Rewriter &R) : TheRewriter(R), reduction_counter(0), barrier_counter(0), new_scope(false), curFunctionDecl(NULL) {}
  bool TraverseStmt(Stmt *Statement) ;
  bool VisitStmt(Stmt *s) ; 
  bool VisitFunctionDecl(FunctionDecl *f) ;
};

// Implementation of the ASTConsumer interface for reading an AST produced
// by the Clang parser.
class MyASTConsumer : public ASTConsumer {
	public:MyASTConsumer(Rewriter &R) : Visitor(R) {}

	       // Override the method that gets called for each parsed top-level
	       // declaration.
	       virtual bool HandleTopLevelDecl(DeclGroupRef DR) {
		       for (DeclGroupRef::iterator b = DR.begin(), e = DR.end(); b != e; ++b)
			       // Traverse the declaration using our AST visitor.
			       Visitor.TraverseDecl(*b);
		       return true;
		}

	private:
		MyASTVisitor Visitor;
};
#endif
