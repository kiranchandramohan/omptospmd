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

#include "clang/AST/ASTContext.h"
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

enum PType
{
	A9,
	M3,
	DSP
} ;

using namespace clang;

struct BufferInfo
{
	int id ;
	int dim1 ;
	int dim2 ;
	int dim3 ;
	std::string arrayName ;
	std::string base_type_str ;
	std::string qualifier_str ;
} ;

struct BarrierInfo
{
	SourceLocation SL ;
	bool printFlush ;
	bool printBarrier ;
} ;

struct varInfo
{
	std::string newName ;
	FunctionDecl* scope ; //KCFIX : Should actually be Scope
} ;

// By implementing RecursiveASTVisitor, we can specify which AST nodes
// we're interested in by overriding relevant methods.
class MyASTVisitor : public RecursiveASTVisitor<MyASTVisitor> {
private:
  Rewriter &TheRewriter ;
  ASTContext& AC ;
  PType PT ;
  unsigned int reduction_counter ;
  unsigned int barrier_counter ;
  bool new_scope ;
  bool inMain ;
  FunctionDecl* curFunctionDecl ;
  FunctionDecl* mainFunctionDecl ;
  Stmt* curOmpFor ;
  SourceLocation MinSL ;
  std::vector<BufferInfo> buffers ;
  std::map<std::string, int> nameToBufferId ;
  std::map<std::string, varInfo> renameMap ;
  std::set<VarDecl*> globalMem ;
  std::set<Stmt*> barrierPresentAtScope ;
  std::vector<Stmt*> StmtStack ;
  std::set<Stmt*> stmt_seen ;
  std::set<unsigned int> renameIgnore ;
  std::set<unsigned int> ompFunction ;
  std::vector<BarrierInfo> barrierInformation ;

  void removePragma(Stmt* s) ;

  void insertStartEndIndxInFor(Stmt* s) ;

  //KCFIX : This is very dumb
  void insertBarrier(Stmt* s) ;

  //KCFIX : Has to be rewritten to reconstruct the prototype rather than inserting
  void insertStartEndIndxInFn(FunctionDecl *f) ;

  //KCFIX : Has to be rewritten to reconstruct the prototype rather than inserting
  void handleReduction(Stmt* s) ;
  void HandleCriticalDirective(Stmt* s) ;
  void GetCriticalDirectiveInfo(Stmt* s, bool& is_array, std::string& critical_var_name, std::string& type_str, BinaryOperatorKind& bok) ;
  void getBufferInfo(VarDecl* v, BufferInfo& bi) ;
  void printVarAsString(VarDecl* v, std::stringstream& SSAfter) ;
  void printBufferProperties() ;
  std::string getBarrierStr() ;
  void printFlushesAndBarriers() ;
  std::string getType(VarDecl* v, bool withName) ;

public:
  MyASTVisitor(Rewriter &R, ASTContext& actxt, PType p) : TheRewriter(R), AC(actxt), PT(p), reduction_counter(0), barrier_counter(0), new_scope(false), inMain(false), 
  						curFunctionDecl(NULL), mainFunctionDecl(NULL), curOmpFor(NULL), MinSL(), buffers(), nameToBufferId(), renameMap(), 
						globalMem(), barrierPresentAtScope(), StmtStack(), stmt_seen(), renameIgnore(), ompFunction(), barrierInformation() { }
  bool TraverseStmt(Stmt *Statement) ;
  bool VisitStmt(Stmt *s) ; 
  bool VisitFunctionDecl(FunctionDecl *f) ;
  bool VisitVarDecl(VarDecl* v) ;
  //bool VisitDecl(Decl* d) ;
};

// Implementation of the ASTConsumer interface for reading an AST produced
// by the Clang parser.
class MyASTConsumer : public ASTConsumer {
	public:MyASTConsumer(Rewriter &R, ASTContext& actxt, PType p) : Visitor(R,actxt,p) {}

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
