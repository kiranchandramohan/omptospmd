//------------------------------------------------------------------------------
// OpenMP to SPMD translator
// Using code(rewritersample.cpp) from Eli Bendersky (eliben@gmail.com) as starting point
// This code is in the public domain
//------------------------------------------------------------------------------
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

  void removePragma(Stmt* s) {
	  SourceLocation sloc_start = s->getLocStart() ;
	  SourceLocation sloc_end = s->getLocEnd() ;
	  SourceLocation sloc_start_new = sloc_start.getLocWithOffset(-(sizeof("#pragma"))) ;
	  CharSourceRange csr = CharSourceRange::getCharRange(sloc_start_new, sloc_end) ;
	  TheRewriter.RemoveText(csr) ;
  }

  void insertStartEndIndxInFor(Stmt* s) {
	  const Stmt *Body = static_cast<OMPForDirective*>(s)->getAssociatedStmt();
	  if (const CapturedStmt *CS = dyn_cast_or_null<CapturedStmt>(Body))
		  Body = CS->getCapturedStmt();
	  bool SkippedContainers = false;
	  const ForStmt* Fstmt1 = dyn_cast_or_null<ForStmt>(Body) ;
	  const Stmt* init = Fstmt1->getInit() ;
	  const Expr* inite1 ;
	  const BinaryOperator* bo1 ;
	  if(inite1=static_cast<const Expr*>(init)) {
		  if(bo1=static_cast<const BinaryOperator*>(inite1)) {
			  if(bo1->isAssignmentOp()) {
				  Expr* rhs1 = bo1->getRHS() ;
				  SourceLocation rhs_loc_start = rhs1->getLocStart() ;
				  SourceLocation rhs_loc_end = rhs1->getLocEnd() ;
				  SourceRange sr(rhs_loc_start, rhs_loc_end) ;
				  TheRewriter.ReplaceText(rhs1->getLocStart(),"start_indx") ;
			  }
		  }
	  }
	  const Expr* cond2 = Fstmt1->getCond() ;
	  const BinaryOperator* bo2 ;
	  if(bo2=static_cast<const BinaryOperator*>(cond2)) {
		  if(bo2->isComparisonOp()) {
			  Expr* rhs2 = bo2->getRHS() ;
			  SourceManager& sm = TheRewriter.getSourceMgr() ;

			  SourceLocation rhs_loc_start = rhs2->getLocStart() ;
			  SourceLocation rhs_loc_end = rhs2->getLocEnd() ;
			  if(sm.isMacroBodyExpansion(rhs_loc_start) && sm.isAtStartOfImmediateMacroExpansion(rhs_loc_start)) {
				  rhs_loc_start = sm.getExpansionRange(rhs_loc_start).first ;
				  rhs_loc_end = sm.getExpansionRange(rhs_loc_end).second ;
			  }
			  SourceRange sr(rhs_loc_start, rhs_loc_end) ;
			  TheRewriter.ReplaceText(sr,"end_indx") ;
		  }
	  }
  }

  //KCFIX : This is very dumb
  void insertBarrier(Stmt* s)
  {
	  std::stringstream barrierStream1 ;
	  SourceLocation ST ;
	  Stmt *Body = static_cast<OMPForDirective*>(s)->getAssociatedStmt();
	  CapturedStmt* CS = NULL ;
	  if (CS = dyn_cast_or_null<CapturedStmt>(Body))
	  	  Body = CS->getCapturedStmt();
	  if(isa<ForStmt>(Body)) {
		  Stmt* scope_stmt = StmtStack[StmtStack.size()-4] ;
		  //printf("Scope = %p\n", static_cast<void *>(scope_stmt)) ;
		  if(barrierPresentAtScope.find(scope_stmt) == barrierPresentAtScope.end()) {
			  ST = Body->getLocStart() ; //.getLocWithOffset(-1) ;
			  barrierStream1<<"call_barrier("<<(barrier_counter++)<<") ;\n" ;
			  TheRewriter.InsertText(ST, barrierStream1.str(),false,true);
		  }
		  ST = CS->getLocEnd().getLocWithOffset(1) ; //KCFIX : Why offset of 3 ?
		  std::stringstream barrierStream2 ;
		  barrierStream2<<"\ncall_barrier("<<(barrier_counter++)<<") ;\n" ;
		  TheRewriter.InsertText(ST, barrierStream2.str(),true,true) ;
		  barrierPresentAtScope.insert(scope_stmt) ;
		  new_scope = false ;
	  }
  }

  //KCFIX : Has to be rewritten to reconstruct the prototype rather than inserting
  void insertStartEndIndxInFn(FunctionDecl *f) {
	  Stmt* s = f->getBody() ;
	  bool is_function_with_omp = false ;
	  if(isa<CompoundStmt>(s)) {
		  CompoundStmt *FuncBody = static_cast<CompoundStmt*>(s) ;

		  Stmt* omp_stmt = NULL ;
		  for(clang::CompoundStmt::body_iterator it=FuncBody->body_begin() ; it!=FuncBody->body_end() ; it++) {
			  Stmt* st = *it ;
			  if(st && isa<OMPForDirective>(st)) {
				  is_function_with_omp = true ;
				  omp_stmt = st ;
				  break ;
			  }
		  }

		  if(is_function_with_omp) {
			  QualType QT = f->getResultType() ;
			  std::string TypeStr = QT.getAsString() ;
			  DeclarationName DeclName = f->getNameInfo().getName() ;
			  std::string FuncName = DeclName.getAsString() ;
			  std::string prototype = TypeStr + " " + FuncName + "(" ;
			  SourceLocation ST = f->getLocStart().getLocWithOffset(prototype.length()) ;
			  std::stringstream SSAfter ;
			  SSAfter << "int start_indx, int end_indx" ;
			  if(f->param_size()>0) {
				  SSAfter<<", " ;
			  }
			  TheRewriter.InsertText(ST, SSAfter.str(), true, true) ;
		  }
	  }
	  DeclarationName DeclName = f->getNameInfo().getName();
	  std::string FuncName = DeclName.getAsString();
	  Stmt *FuncBody = f->getBody();
	  std::stringstream SSAfter;
	  if(is_function_with_omp) {
		  SSAfter << "// End modified function " << FuncName;
	  } else {
		  SSAfter << "// End function " << FuncName;
	  }
	  SourceLocation ST = FuncBody->getLocEnd().getLocWithOffset(1);
	  TheRewriter.InsertText(ST, SSAfter.str(), true, true);
  }

  //KCFIX : Has to be rewritten to reconstruct the prototype rather than inserting
  void handleReduction(Stmt* s)
  {
	  OMPForDirective* ofd = static_cast<OMPForDirective*>(s) ;
	  for(int i=0 ; i<ofd->getNumClauses() ; i++) {
		  OMPClause* oc = ofd->getClause(i) ;
		  if(isa<OMPReductionClause>(oc)) {
			  OMPReductionClause* orc = static_cast<OMPReductionClause*>(oc) ;
			  //printf("OMP reduction clause seen\n") ;
			  //printf("ORC = %s\n",orc->getOpName().getName().getAsString().c_str()) ;
			  std::stringstream redStream ;
			  redStream<<"\n\ncall_reduce("<<(reduction_counter++)<<"," ;
			  if(orc->getOperator() == OMPC_REDUCTION_add) {
				  redStream<<"ADD," ;
			  } else if(orc->getOperator() == OMPC_REDUCTION_mult) {
				  redStream<<"MULT," ;
			  } else if(orc->getOperator() == OMPC_REDUCTION_min) {
				  redStream<<"MIN," ;
			  } else if(orc->getOperator() == OMPC_REDUCTION_max) {
				  redStream<<"MAX," ;
			  } else {
				  printf("Unsupported reduction Kind\n") ;
				  exit(-1) ;
			  }
			  std::string data_str ;
			  for(ArrayRef<const Expr *>::iterator it=orc->varlist_begin() ; it!=orc->varlist_end() ; it++) {
				  const Expr* e = *it ;
				  if(isa<DeclRefExpr>(e)) {
					  data_str = static_cast<const DeclRefExpr*>(e)->getDecl()->getName() ;
					  redStream<<"&"<<data_str ;
					  break ;
				  }
			  }
			  redStream<<") ; \n" ;

			  std::stringstream barrierStream ;
			  barrierStream<<"\ncall_barrier("<<(barrier_counter++)<<") ;\n" ;
			  std::stringstream redStream2 ;
			  redStream2<<data_str<<" = read_reduce("<<(reduction_counter-1)<<") ;\n" ;

			  const Stmt *Body = static_cast<OMPForDirective*>(s)->getAssociatedStmt();
			  if (const CapturedStmt *CS = dyn_cast_or_null<CapturedStmt>(Body))
				  Body = CS->getCapturedStmt();
			  const ForStmt* Fstmt = dyn_cast_or_null<ForStmt>(Body) ;
			  SourceLocation ST1 = Fstmt->getLocEnd().getLocWithOffset(1) ;
			  TheRewriter.InsertText(ST1, redStream.str(), true, true);
			  TheRewriter.InsertText(ST1, barrierStream.str(), true, true);
			  TheRewriter.InsertText(ST1, redStream2.str(), true, true);
		  }
	  }
  }

public:
  MyASTVisitor(Rewriter &R) : TheRewriter(R), reduction_counter(0), barrier_counter(0), new_scope(false) {}

  bool TraverseStmt(Stmt *Statement) {
	  //printf("Start visiting statement %p\n", Statement) ;
	  StmtStack.push_back(Statement) ;
	  RecursiveASTVisitor<MyASTVisitor>::TraverseStmt(Statement) ;
	  StmtStack.pop_back() ;
	  //printf("End visiting statement %p\n", Statement) ;
	  return true ;
  }

  bool VisitStmt(Stmt *s) { 
	  if(isa<OMPForDirective>(s)) {
		  //printf("Seeing an OMP for directive\n") ;
		  std::set<Stmt*>::const_iterator got = stmt_seen.find(s) ;
		  if(got == stmt_seen.end()) {
			  stmt_seen.insert(s) ;
			  removePragma(s) ;
			  insertStartEndIndxInFor(s) ;
			  insertBarrier(s) ;
			  handleReduction(s) ;
		  }
	  }

	  if(isa<OMPCriticalDirective>(s)) {
		  printf("OMP critical directive\n") ;
		  if(static_cast<OMPCriticalDirective*>(s)->hasAssociatedStmt()) {
			  printf("OMP critical has associated statement\n") ;
			  Stmt* s1 = static_cast<OMPCriticalDirective*>(s)->getAssociatedStmt() ;
			  if(isa<CapturedStmt>(s1)) {
				  printf("OMP critical captured statement\n") ;
				  CapturedStmt* s2 = static_cast<CapturedStmt*>(s1) ;
				  Stmt* s3 = s2->getCapturedStmt() ;
				  if(isa<BinaryOperator>(s3)) {
					  BinaryOperator* s4 = static_cast<BinaryOperator*>(s3) ;
					  if(s4->isCompoundAssignmentOp()) {
						  printf("Compound Assignment Op\n") ;
						  Opcode opc = s4->getOpForCompoundAssignment(s4->getOpcode()) ;
						  //if(opc == BO_Add) ;
						  printf("Add compound assignment\n") ;
						  Expr* lhs = s4->getLHS() ;
						  if(isa<ArraySubscriptExpr>(lhs)) {
							  printf("Array Subscript Expr\n") ;
							  Expr* gb = static_cast<ArraySubscriptExpr*>(lhs)->getBase() ;
							  if(isa<ImplicitCastExpr>(gb)) {
								  printf("Implicit cast expression\n") ;
								  Expr* se = static_cast<ImplicitCastExpr*>(gb)->getSubExpr() ;
								  if(isa<DeclRefExpr>(se)) {
									  printf("DeclRef expression\n") ;
									  printf("Array name = %s\n",static_cast<const DeclRefExpr*>(se)->getDecl()->getName().data()) ;
								  }
							  }
						  } else if(isa<DeclRefExpr>(lhs)) {
							  printf("DeclRef expression\n") ;
							  printf("Variable name = %s\n",static_cast<const DeclRefExpr*>(lhs)->getDecl()->getName().data()) ;
						  } else {
							  printf("Tool Limitation : Currently Critical Section Directive expects operation on either an array or varialble\n") ;
							  exit(-1) ;
						  }
					  } else {
						  printf("Tool Limitation : Currently Critical Directive only supported for Compound Assignments\n") ;
						  exit(-1) ;
					  }
				  }
			  }
		  }
	  }

	  if(isa<ForStmt>(s)) {
		  Stmt* scope_stmt = StmtStack[StmtStack.size()-2] ;
		  std::set<Stmt*>::iterator it = barrierPresentAtScope.find(scope_stmt) ;
		  if(it != barrierPresentAtScope.end())
			  barrierPresentAtScope.erase(it);
	  }
	  return true;
  }

  bool VisitFunctionDecl(FunctionDecl *f) {
	  // Only function definitions (with bodies), not declarations.
	  if (f->hasBody()) {
		  insertStartEndIndxInFn(f) ;
	  }
	  return true;
  }
};

// Implementation of the ASTConsumer interface for reading an AST produced
// by the Clang parser.
class MyASTConsumer : public ASTConsumer {
	public:
		MyASTConsumer(Rewriter &R) : Visitor(R) {}

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

int main(int argc, char *argv[]) {
	if (argc != 2) {
		llvm::errs() << "Usage: rewritersample <filename>\n";
		return 1;
	}

	// Arguments to pass to the clang frontenda
	std::string inputPath("-fopenmp") ;
	std::vector<const char *> args;
	args.push_back(inputPath.c_str());
	// The compiler invocation needs a DiagnosticsEngine so it can report problems
	DiagnosticOptions dgo = clang::DiagnosticOptions() ;

	IntrusiveRefCntPtr<DiagnosticOptions> DiagOpts = new DiagnosticOptions();
	TextDiagnosticPrinter *DiagClient = new TextDiagnosticPrinter(llvm::errs(), &*DiagOpts);
	llvm::IntrusiveRefCntPtr<clang::DiagnosticIDs> DiagID(new clang::DiagnosticIDs());
	//clang::DiagnosticsEngine Diags(DiagID, DiagClient);
	clang::DiagnosticsEngine Diags(DiagID, &*DiagOpts, DiagClient);

	// Create the compiler invocation
	llvm::OwningPtr<clang::CompilerInvocation> CI(new clang::CompilerInvocation);
	clang::CompilerInvocation::CreateFromArgs(*CI, &args[0], &args[0] + args.size(), Diags);

	// CompilerInstance will hold the instance of the Clang compiler for us,
	// managing the various objects.
	CompilerInstance TheCompInst;
	//TheCompInst.createDiagnostics(argc,argv);
	TheCompInst.createDiagnostics();
	TheCompInst.setInvocation(CI.take()) ;

	// Initialize target info with the default triple for our platform.
	TargetOptions *TO = new TargetOptions;
	TO->Triple = llvm::sys::getDefaultTargetTriple();
	TargetInfo *TI =
		TargetInfo::CreateTargetInfo(TheCompInst.getDiagnostics(), TO);
	TheCompInst.setTarget(TI);

	TheCompInst.createFileManager();
	FileManager &FileMgr = TheCompInst.getFileManager();
	TheCompInst.createSourceManager(FileMgr);
	SourceManager &SourceMgr = TheCompInst.getSourceManager();
	TheCompInst.createPreprocessor();
	TheCompInst.createASTContext();

	// A Rewriter helps us manage the code rewriting task.
	Rewriter TheRewriter;
	TheRewriter.setSourceMgr(SourceMgr, TheCompInst.getLangOpts());

	//// Set the main file handled by the source manager to the input file.
	const FileEntry *FileIn = FileMgr.getFile(argv[1]);
	if (!FileIn) {
		llvm::errs() << "Input file does not exist!\n";
		return 1;
	}
	SourceMgr.createMainFileID(FileIn);
	TheCompInst.getDiagnosticClient().BeginSourceFile(
			TheCompInst.getLangOpts(), &TheCompInst.getPreprocessor());

	// Create an AST consumer instance which is going to get called by
	// ParseAST.
	MyASTConsumer TheConsumer(TheRewriter);

	// Parse the file to AST, registering our consumer as the AST consumer.
	ParseAST(TheCompInst.getPreprocessor(), &TheConsumer,
			TheCompInst.getASTContext());

	//// At this point the rewriter's buffer should be full with the rewritten
	//// file contents.
	const RewriteBuffer *RewriteBuf =
		TheRewriter.getRewriteBufferFor(SourceMgr.getMainFileID());
	llvm::outs() << std::string(RewriteBuf->begin(), RewriteBuf->end());

	return 0;
}
