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
#include <cmath>

#include "clang/AST/ASTConsumer.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/AST/OpenMPClause.h"
#include "clang/AST/GlobalDecl.h"
#include "clang/Basic/Diagnostic.h"
#include "clang/Basic/FileManager.h"
#include "clang/Basic/SourceManager.h"
#include "clang/Basic/TargetOptions.h"
#include "clang/Basic/TargetInfo.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Frontend/TextDiagnosticPrinter.h"
#include "clang/Frontend/Utils.h"
#include "clang/Lex/Preprocessor.h"
#include "clang/Parse/ParseAST.h"
#include "clang/Rewrite/Core/Rewriter.h"
#include "clang/Rewrite/Frontend/Rewriters.h"
#include "clang/AST/OperationKinds.h"
#include "llvm/Support/Host.h"
#include "llvm/Support/raw_ostream.h"
#include "omptospmd.h" 
#include "DefUse.h"

using namespace clang;

void MyASTVisitor::printBufferProperties()
{
	if(PT == A9) {
		std::stringstream SSAfter ;
		SSAfter<<"void set_buffer_properties(struct buffer* bufs, int nbuf)" ;
		SSAfter<<"\n{" ;
		SSAfter<<"\n\tint i ;" ;
		SSAfter<<"\n\tfor(i=0 ; i<nbuf ; i++) {" ;
		SSAfter<<"\n\t\tbufs[i].cache_type = OMAP_BO_CACHED ;" ;
		SSAfter<<"\n\t\tbufs[i].privatize = false ;" ;
		SSAfter<<"\n\t\tbufs[i].rd_only = false ;" ;
		SSAfter<<"\n\t\tbufs[i].nbo = 1 ;" ;
		SSAfter<<"\n\t\tbufs[i].remote_attach = true ;" ;
		if(buffers.size() > 0) {
			if(buffers[0].dim1>4096 && buffers[0].dim2==1) {
				buffers[0].dim2 = buffers[0].dim1 = static_cast<int>(sqrt(static_cast<double>(buffers[0].dim1))) ;
			}
		}
		SSAfter<<"\n\t\tbufs[i].width = "<<((buffers.size()>0)?buffers[0].dim1:1)<<" ;" ;
		SSAfter<<"\n\t\tbufs[i].height = "<<((buffers.size()>0)?buffers[0].dim2:1)<<" ;" ;
		SSAfter<<"\n\t}" ;
		SSAfter<<"\n}" ;
		SourceLocation ST = MinSL.getLocWithOffset(-1);
		TheRewriter.InsertText(ST, SSAfter.str(), true, true);
	}
}

void MyASTVisitor::getDimensions(VarDecl* v, BufferInfo& bi)
{
	bi.arrayName = v->getNameAsString() ;
	bi.dim1 = bi.dim2 = bi.dim3 = 1 ;
	QualType QT1 = v->getTypeSourceInfo() ? v->getTypeSourceInfo()->getType() : v->getType() ;
	const Type* T1 = QT1.getTypePtr() ;
	if(T1->isConstantArrayType()) {
		const ConstantArrayType* cat1 = dyn_cast<ConstantArrayType>(T1) ; 
		bi.dim1 = cat1->getSize().getSExtValue() ;
		QualType QT2 = cat1->getElementType() ;
		const Type* T2 = QT2.getTypePtr() ;
		if(T2 && T2->isConstantArrayType()) {
			const ConstantArrayType* cat2 = dyn_cast<ConstantArrayType>(T2) ;
			bi.dim2 = cat2->getSize().getSExtValue() ;
			QualType QT3 = cat2->getElementType() ;
			const Type* T3 = QT3.getTypePtr() ;
			const Type* T4 ;
			if(T3 && T3->isConstantArrayType()) {
				const ConstantArrayType* cat3 = dyn_cast<ConstantArrayType>(T3) ;
				bi.dim3 = cat3->getSize().getSExtValue() ;
				T4 = T3->getBaseElementTypeUnsafe();
			} else {
				T4 = T3->getBaseElementTypeUnsafe();
			}
			if(T4->isBuiltinType()) {
				bi.base_type_str = dyn_cast<BuiltinType>(T4)->getName(AC.getPrintingPolicy()) ;
			}
		} else {
			const Type* T3 = T1->getBaseElementTypeUnsafe();
			if(T3->isBuiltinType()) {
				bi.base_type_str = dyn_cast<BuiltinType>(T3)->getName(AC.getPrintingPolicy()) ;
			}
		}
	} else if(T1->isPointerType()) {
		const Type* T3 = T1->getPointeeType().getTypePtr() ;
		if(T3->isBuiltinType()) {
			bi.base_type_str = dyn_cast<BuiltinType>(T3)->getName(AC.getPrintingPolicy()) ;
		}
	}

	if(v->getType().isRestrictQualified()) {
		if(PT == A9) {
			bi.qualifier_str = "__restrict__" ;
		} else {//M3, DSP
			bi.qualifier_str = "restrict" ;
		}
	}
}

std::string MyASTVisitor::getType(VarDecl* v, bool withName)
{
	std::stringstream SSAfter ;
	int dim1 ;
	int dim2 ;
	std::string base_type_str ;
	std::string qualifier_str ;
	BufferInfo bi ;
	getDimensions(v,bi) ;
	SSAfter<<bi.base_type_str ;
	std::string varName ;
	if(withName) {
		varName = bi.arrayName ;
	} else {
		varName = "" ;
	}
	if(bi.dim2 > 1)
		SSAfter<<"(" ;
	SSAfter<<"*"<<bi.qualifier_str<<" "<<varName ;
	if(bi.dim2 > 1)
		SSAfter<<")["<<bi.dim2<<"]" ;
	if(bi.dim3 > 1)
		SSAfter<<"["<<bi.dim3<<"]" ;
	return SSAfter.str() ;
}

void MyASTVisitor::printVarAsString(VarDecl* v, std::stringstream& SSAfter)
{
	SSAfter<<getType(v, true) ;
}
			

// By implementing RecursiveASTVisitor, we can specify which AST nodes
// we're interested in by overriding relevant methods.
void MyASTVisitor::removePragma(Stmt* s)
{
	SourceLocation sloc_start = s->getLocStart() ;
	SourceLocation sloc_end = s->getLocEnd() ;
	SourceLocation sloc_start_new = sloc_start.getLocWithOffset(-(sizeof("#pragma"))) ;
	CharSourceRange csr = CharSourceRange::getCharRange(sloc_start_new, sloc_end) ;
	TheRewriter.RemoveText(csr) ;
}

void MyASTVisitor::insertStartEndIndxInFor(Stmt* s)
{
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

std::string MyASTVisitor::getBarrierStr()
{
	std::stringstream barrierStream ;
	if(PT == A9) {
		barrierStream<<"\nif(tid == 0) {" ;
		barrierStream<<"\n\tcall_barrier("<<barrier_counter<<") ;" ;
		barrierStream<<"\n}" ;
		barrierStream<<"\npthread_barrier_wait(&barrier"<<barrier_counter<<") ;" ;
	} else if(PT == M3) {
		barrierStream<<"\nif(tid == 0) {" ;
		barrierStream<<"\n\tcallBarrier("<<barrier_counter<<", /*lock_id=*/4) ;" ;
		barrierStream<<"\n}" ;
		barrierStream<<"\ncallLocalBarrier() ;" ;
	} else if(PT == DSP) {
		barrierStream<<"\n\tcallBarrier("<<barrier_counter<<", /*lock_id=*/4) ;" ;
	} else {
	}
	barrier_counter++ ;

	return barrierStream.str() ;
}

void MyASTVisitor::printFlushesAndBarriers()
{
	for (std::vector<BarrierInfo>::iterator it = barrierInformation.begin() ; it != barrierInformation.end(); it++)
	{
		std::stringstream flushStream ;
		if((*it).printFlush) {
			if(PT == A9) {
				flushStream<<"\nif(tid == 0) {" ;
				flushStream<<"\n\tif(remote_fd.sysm3 >= 0) {" ;
				for(int i=0 ; i<buffers.size() ; i++) {
					flushStream<<"\n\t\tdetach_buffer(buffers["<<i<<"], remote_fd.sysm3, 0) ;" ;
					flushStream<<"\n\t\tattach_buffer(buffers["<<i<<"], remote_fd.sysm3, 0) ;" ;
				}
				flushStream<<"\n\t}" ;
				flushStream<<"\n\tif(remote_fd.dsp >= 0) {" ;
				for(int i=0 ; i<buffers.size() ; i++) {
					flushStream<<"\n\t\tdetach_buffer(buffers["<<i<<"], remote_fd.dsp, 0) ;" ;
					flushStream<<"\n\t\tattach_buffer(buffers["<<i<<"], remote_fd.dsp, 0) ;" ;
				}
				flushStream<<"\n\t}" ;
				flushStream<<"\n}\n" ;
			}
		}
		if((*it).printBarrier)
			flushStream<<getBarrierStr()<<"\n" ;
		TheRewriter.InsertText((*it).SL, flushStream.str(),true,true) ;
	}
}

//KCFIX : This is very dumb
void MyASTVisitor::insertBarrier(Stmt* s)
{
	std::stringstream barrierStream1 ;
	Stmt *Body = static_cast<OMPForDirective*>(s)->getAssociatedStmt();
	CapturedStmt* CS = NULL ;
	if (CS = dyn_cast_or_null<CapturedStmt>(Body))
		Body = CS->getCapturedStmt();
	if(isa<ForStmt>(Body)) {
		Stmt* scope_stmt = StmtStack[StmtStack.size()-4] ;
		//printf("Scope = %p\n", static_cast<void *>(scope_stmt)) ;
		if(barrierPresentAtScope.find(scope_stmt) == barrierPresentAtScope.end()) {
			BarrierInfo BI ;
			BI.printBarrier = BI.printFlush = true ;
			BI.SL = Body->getLocStart() ; //.getLocWithOffset(-1) ;
			//barrierStream1<<getBarrierStr()<<"\n" ;
			TheRewriter.InsertText(BI.SL, barrierStream1.str(),false,true);
			barrierInformation.push_back(BI) ;
		}
		BarrierInfo BI ;
		BI.printBarrier = BI.printFlush = true ;
		BI.SL = CS->getLocEnd().getLocWithOffset(1) ; //KCFIX : Why offset of 3 ?
		std::stringstream barrierStream2 ;
		//barrierStream2<<getBarrierStr() ;
		barrierInformation.push_back(BI) ;
		TheRewriter.InsertText(BI.SL, barrierStream2.str(),true,true) ;
		barrierPresentAtScope.insert(scope_stmt) ;
		new_scope = false ;
	}
}

  //KCFIX : Has to be rewritten to reconstruct the prototype rather than inserting
void MyASTVisitor::insertStartEndIndxInFn(FunctionDecl *f)
{
	Stmt* s = f->getBody() ;
	bool is_function_with_omp = false ;
	if(isa<CompoundStmt>(s)) {
		CompoundStmt *FuncBody = static_cast<CompoundStmt*>(s) ;

		Stmt* omp_stmt = NULL ;
		for(clang::CompoundStmt::body_iterator it=FuncBody->body_begin() ; it!=FuncBody->body_end() ; it++) {
			Stmt* st = *it ;
			if(st && (isa<OMPForDirective>(st) || isa<OMPParallelDirective>(st))) {
				is_function_with_omp = true ;
				omp_stmt = st ;
				break ;
			}
		}

		if(is_function_with_omp) {
			ompFunction.insert(f->getLocStart().getRawEncoding()) ;
			QualType QT = f->getResultType() ;
			std::string TypeStr = QT.getAsString() ;
			std::string FuncName = f->getNameAsString() ;
			std::stringstream SSAfter ;
			SSAfter<<TypeStr<<" "<<FuncName<<"(" ;
			for(FunctionDecl::param_iterator it=f->param_begin() ; it!=f->param_end() ; it++) {
				//std::string Str;
				//llvm::raw_string_ostream S(Str);
				//(*it)->print(S) ;
				//if(const ArrayType* ATy = dyn_cast<ArrayType>((*it)->getOriginalType())) {
				//	SSAfter<<ATy->getElementType().getAsString().c_str()<<" "<<(*it)->getNameAsString().c_str()<<", " ;
				//} else {
				//	SSAfter<<(*it)->getOriginalType().getAsString().c_str()<<" "<<(*it)->getNameAsString().c_str()<<", " ;
				//}
				printVarAsString((*it), SSAfter) ;
				SSAfter<<", " ;
				//SSAfter2<<"\n\t"<<base_type_str ;
				//if((dim1 > 1) && (dim2 > 1)) {
				//	SSAfter2<<"(*"<<(*got)->getNameAsString().c_str()<<")["<<dim2<<"]" ;
			
			}
			if(PT == A9) {
				SSAfter << "struct buffer* __restrict__ buffers, int tid, int start_indx, int end_indx)" ;
			} else if(PT == M3) {
				SSAfter << "int tid, int start_indx, int end_indx)" ;
			} else if(PT == M3 || PT == DSP) {
				SSAfter << "int start_indx, int end_indx)" ;
			}
			//SourceLocation ST = f->getLocStart().getLocWithOffset(prototype.length()) ;
			SourceRange SR(f->getLocStart(), s->getLocStart().getLocWithOffset(-1)) ;
			TheRewriter.ReplaceText(SR, SSAfter.str()) ;
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
void MyASTVisitor::handleReduction(Stmt* s)
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
					redStream<<data_str ;
					break ;
				}
			}
			redStream<<") ; \n" ;

			std::stringstream barrierStream ;
			barrierStream<<"\ncall_barrier("<<(barrier_counter++)<<") ;\n" ;
			std::stringstream redStream2 ;
			redStream2<<data_str<<" = call_read_reduce("<<(reduction_counter-1)<<") ;\n" ;

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

bool MyASTVisitor::TraverseStmt(Stmt *Statement)
{
	//printf("Start visiting statement %p\n", Statement) ;
	StmtStack.push_back(Statement) ;
	RecursiveASTVisitor<MyASTVisitor>::TraverseStmt(Statement) ;
	StmtStack.pop_back() ;
	//printf("End visiting statement %p\n", Statement) ;
	return true ;
}

//KCFIX : Currently critical works only for one directive
void MyASTVisitor::HandleCriticalDirective(Stmt* s)
{
	bool is_array = false ;
	std::string critical_var_name ;
	std::string type_str ;
	BinaryOperatorKind bok ;
	GetCriticalDirectiveInfo(s, is_array, critical_var_name, type_str, bok) ;
	int val = is_array?1:0 ;
	//printf("Is array=%d, var name=%s, type=%s\n", val, critical_var_name.c_str(), type_str.c_str()) ;
	std::string array_size = "PRIVATIZE_ARRAY_SIZE" ;
	std::string new_name = "my_" + critical_var_name ;
	std::string glob_name = "glob_" + critical_var_name ;
	std::stringstream prvStream ;
	if(is_array) {
		SourceLocation SL = curFunctionDecl->getLocStart().getLocWithOffset(-1) ;
		std::stringstream fdlStream ;
		fdlStream<<type_str<<" (*"<<glob_name<<")[NUM_TOTAL_THREADS] ;\n" ;
		TheRewriter.InsertText(SL, fdlStream.str(), true, true);

		prvStream<<"\n"<<type_str<<" "<<new_name<<"["<<array_size<<"] ;\n" ;
	} else {
		prvStream<<"\n"<<type_str<<" "<<new_name<<" ;\n" ;
	}
	prvStream<<"for (i = 0; i < NUM_TOTAL_THREADS; i++) {\n" ;
	if(is_array) {
		prvStream<<"\tfor(j=0 ; j<"<<array_size<<" ; j++) {\n" ;
		prvStream<<"\t\tif(i==0)\n" ;
		prvStream<<"\t\t\t"<<new_name<<"[j] = ("<<glob_name<<"[i])[j] ;\n" ;
		prvStream<<"\t\telse\n" ;
		prvStream<<"\t\t\t"<<new_name<<"[j]" ;
		if(bok == BO_Add) {
			prvStream<<" += " ;
		} else {
			printf("Tool Limitation : Currently Critical Directive only supported for Addition\n") ;
			exit(-1) ;
		}
		prvStream<<"("<<glob_name<<"[i])[j] ;\n" ;
		prvStream<<"\t}\n" ;
	} else {
		prvStream<<"\t"<<new_name ;
		if(bok == BO_Add) {
			prvStream<<" += " ;
		} else {
			printf("Tool Limitation : Currently Critical Directive only supported for Addition\n") ;
			exit(-1) ;
		}
		prvStream<<glob_name<<"[i] ; \n" ;
	}
	prvStream<<"}\n" ;
	Stmt *Body = static_cast<OMPForDirective*>(curOmpFor)->getAssociatedStmt();
	CapturedStmt* CS = NULL ;
	if (CS = dyn_cast_or_null<CapturedStmt>(Body))
		Body = CS->getCapturedStmt();
	SourceLocation ST1 = CS->getLocEnd().getLocWithOffset(1) ;
	TheRewriter.InsertText(ST1, prvStream.str(), true, true);
	renameMap[critical_var_name] = new_name ;
}

void MyASTVisitor::GetCriticalDirectiveInfo(Stmt* s, bool& is_array, std::string& critical_var_name, std::string& type_str, BinaryOperatorKind& bok)
{
	if(static_cast<OMPCriticalDirective*>(s)->hasAssociatedStmt()) {
		Stmt* s1 = static_cast<OMPCriticalDirective*>(s)->getAssociatedStmt() ;
		if(isa<CapturedStmt>(s1)) {
			CapturedStmt* s2 = static_cast<CapturedStmt*>(s1) ;
			Stmt* s3 = s2->getCapturedStmt() ;
			if(isa<BinaryOperator>(s3)) {
				BinaryOperator* s4 = static_cast<BinaryOperator*>(s3) ;
				type_str = s4->getType().getAsString() ;
				if(s4->isCompoundAssignmentOp()) {
					bok = s4->getOpForCompoundAssignment(s4->getOpcode()) ;
					Expr* lhs = s4->getLHS() ;
					if(isa<ArraySubscriptExpr>(lhs)) {
						is_array = true ;
						Expr* gb = static_cast<ArraySubscriptExpr*>(lhs)->getBase() ;
						if(isa<ImplicitCastExpr>(gb)) {
							Expr* se = static_cast<ImplicitCastExpr*>(gb)->getSubExpr() ;
							if(isa<DeclRefExpr>(se)) {
								critical_var_name = static_cast<const DeclRefExpr*>(se)->getDecl()->getName().str() ;
								//printf("Array name = %s\n",static_cast<const DeclRefExpr*>(se)->getDecl()->getName().data()) ;
								//renameIgnore.insert(reinterpret_cast<unsigned int>(se)) ;
								renameIgnore.insert(se->getLocStart().getRawEncoding()) ;
								//printf("Insert Renaming DeclRefExpr %u %s\n", se->getLocStart().getRawEncoding(), static_cast<const DeclRefExpr*>(se)->getDecl()->getName().str().c_str()) ;
							}
						}
					} else if(isa<DeclRefExpr>(lhs)) {
						is_array = false ;
						critical_var_name = static_cast<const DeclRefExpr*>(lhs)->getDecl()->getName().str() ;
						renameIgnore.insert(lhs->getLocStart().getRawEncoding()) ;
						//printf("Insert Renaming DeclRefExpr %u %s\n", lhs->getLocStart().getRawEncoding(), static_cast<const DeclRefExpr*>(lhs)->getDecl()->getName().str().c_str()) ;
						//printf("Variable name = %s\n",static_cast<const DeclRefExpr*>(lhs)->getDecl()->getName().data()) ;
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

bool MyASTVisitor::VisitStmt(Stmt *s)
{ 
	std::set<Stmt*>::const_iterator got = stmt_seen.find(s) ;
	if(got != stmt_seen.end()) {
		return true ;
	}
	stmt_seen.insert(s) ;

	if(isa<OMPForDirective>(s)) {
		curOmpFor = s ;
		//printf("Seeing an OMP for directive\n") ;
		removePragma(s) ;
		insertStartEndIndxInFor(s) ;
		insertBarrier(s) ;
		handleReduction(s) ;
	}

	if(isa<ForStmt>(s)) {
		Stmt* scope_stmt = StmtStack[StmtStack.size()-2] ;
		std::set<Stmt*>::iterator it = barrierPresentAtScope.find(scope_stmt) ;
		if(it != barrierPresentAtScope.end())
			barrierPresentAtScope.erase(it);

		if(inMain) {
			Stmt* sb = (dyn_cast<ForStmt>(s))->getBody() ;
			BarrierInfo BI ;
			BI.printFlush = false ;
			BI.printBarrier = true ;
			BI.SL = sb->getLocStart().getLocWithOffset(1) ;
			barrierInformation.push_back(BI) ;
			if(PT == M3) {
				std::stringstream SSAfter ;
				SourceLocation SL = sb->getLocEnd().getLocWithOffset(2) ;
				SSAfter<<"\n\tif(tid == 0) {" ;
				SSAfter<<"\n\t\tEvent_post(edgeDetectEvent, Event_Id_00) ;" ;
				SSAfter<<"\n\t} else {" ;
				SSAfter<<"\n\t\tEvent_post(edgeDetectEvent, Event_Id_01) ;" ;
				SSAfter<<"\n\t}" ;
				TheRewriter.InsertText(SL, SSAfter.str(), true, true);
			}
		}
	}

	if(isa<OMPCriticalDirective>(s)) {
		HandleCriticalDirective(s) ;
		removePragma(s) ;
	}

	//KCFIX : Limitation call should appear after definition of function
	if(isa<CallExpr>(s)) {
		CallExpr* ce = static_cast<CallExpr*>(s) ;
		FunctionDecl* fd = ce->getDirectCallee() ;
		//printf("Function = %s\n", fd->getNameAsString().c_str()) ;
		std::set<unsigned int>::iterator it = ompFunction.find(fd->getLocStart().getRawEncoding()) ;
		if(it != ompFunction.end()) {
			std::string FuncName = fd->getNameAsString() ;
			std::stringstream SSAfter ;
			SSAfter<<FuncName<<"(" ;
			for(int i=0 ; i<ce->getNumArgs() ; i++) {
				if(isa<DeclRefExpr>(ce->getArg(i))) {
					DeclRefExpr* de = static_cast<DeclRefExpr*>(ce->getArg(i)) ;
					SSAfter<<de->getDecl()->getName().data()<<", " ;
				} else if(isa<ImplicitCastExpr>(ce->getArg(i))) {
					Expr* se = static_cast<ImplicitCastExpr*>(ce->getArg(i))->getSubExpr() ;
					if(isa<DeclRefExpr>(se)) {
						DeclRefExpr* de = static_cast<DeclRefExpr*>(se) ;
						SSAfter<<de->getDecl()->getName().data()<<", " ;
					}
				}
			}
			if(PT == A9) {
				SSAfter << "buffers, tid, start_indx, end_indx)" ;
			} else if(PT == M3) {
				SSAfter << "tid, t->start_indx, t->end_indx)" ;
			} else if(PT == DSP) {
				SSAfter << "args->start_indx, args->end_indx)" ;
			}
			SourceRange SR(s->getLocStart(), s->getLocEnd()) ;
			TheRewriter.ReplaceText(SR, SSAfter.str()) ;
		}
	}

	if(isa<DeclRefExpr>(s)) {
		if(renameIgnore.find(s->getLocStart().getRawEncoding()) == renameIgnore.end()) {
			//printf("Renaming DeclRefExpr %u %s\n", s->getLocStart().getRawEncoding(), static_cast<const DeclRefExpr*>(s)->getDecl()->getName().str().c_str()) ;
			std::string curName = static_cast<const DeclRefExpr*>(s)->getDecl()->getName() ;
			std::map<std::string,std::string>::iterator it = renameMap.find(curName) ;
			if(it != renameMap.end()) {
				SourceRange sr(s->getLocStart(), s->getLocEnd()) ;
				TheRewriter.ReplaceText(sr, (*it).second.c_str()) ;
			}
		} else {
			//printf("Ignoring DeclRefExpr %u %s\n", reinterpret_cast<unsigned int>(s), static_cast<const DeclRefExpr*>(s)->getDecl()->getName().str().c_str()) ;
		}
	}

	if(isa<ReturnStmt>(s)) {
		if(inMain) {
			SourceLocation SL = s->getLocStart() ;
			//TheRewriter.InsertText(SL, "\n}\n", false, true);
		}
	}

	return true;
}

bool MyASTVisitor::VisitFunctionDecl(FunctionDecl *f)
{
	// Only function definitions (with bodies), not declarations.
	if (f->hasBody()) {
		if(MinSL.isValid()) {
			MinSL = MinSL < f->getLocStart() ? MinSL : f->getLocStart() ;
		} else {
			MinSL = f->getLocStart() ;
		}
		curFunctionDecl = f ;
		insertStartEndIndxInFn(f) ;

		inMain = f->isMain() ;
		if(inMain) {
			mainFunctionDecl = f ;
		} else {
			std::string FStr = f->getNameInfo().getAsString() ;
			if(FStr == std::string("cleanup")) {
				Stmt* s = mainFunctionDecl->getBody() ;
				SourceRange SR(mainFunctionDecl->getLocStart(), s->getLocStart().getLocWithOffset(-1)) ;
				std::stringstream SSAfter1 ;
				if(PT == A9) {
					SSAfter1<<"\nint a9_compute(int tid, struct buffer* buffers, int start_indx, int end_indx)" ;
				} else if(PT == M3) {
					SSAfter1<<"\nint common_wrapper(UArg arg0, UArg arg1)" ;

				} else if(PT == DSP) {
					SSAfter1<<"\nint common_wrapper(FxnArgs* args)" ;
				}
				TheRewriter.ReplaceText(SR, SSAfter1.str()) ;

				SourceLocation SL = s->getLocStart().getLocWithOffset(1) ;
				std::stringstream SSAfter2 ;
				if(PT == M3) {
					SSAfter2<<"\n\ttaskArgs* t = (taskArgs*)arg0 ;" ;
					SSAfter2<<"\n\tint tid = (int)arg1 ;" ;
				}
				std::set<VarDecl*>::const_iterator got ;
				int i ;
				for(i=0,got=globalMem.begin() ; got!= globalMem.end() ; got++,i++) {
					SSAfter2<<"\n\t" ;
					printVarAsString((*got), SSAfter2) ;
					if(PT == A9) {
						SSAfter2<<"= ("<<getType((*got),false)<<")buffers["<<i<<"].bo[0]->map ;" ;
					} else if(PT == M3) {
						SSAfter2<<"= ("<<getType((*got),false)<<")t->buffer"<<(i+1)<<" ;" ;
					} else if(PT == DSP) {
						char arg = 'a' + i ;
						SSAfter2<<"= ("<<getType((*got),false)<<")args->"<<arg<<" ;" ;
					}
				}
				TheRewriter.InsertText(SL, SSAfter2.str(), true, true);

				printBufferProperties() ;
				printFlushesAndBarriers() ;
			}
		}
	}

	return true;
}

bool MyASTVisitor::VisitVarDecl(VarDecl* v)
{
	if(v->hasGlobalStorage() || inMain) {
		QualType QT1 = v->getType() ;
		const Type* T1 = QT1.getTypePtr() ;
		if(T1->isConstantArrayType()) {
			std::set<VarDecl*>::const_iterator got = globalMem.find(v) ;
			if(got == globalMem.end()) {
				globalMem.insert(v) ;

				BufferInfo bi ;
				std::string dummy_base_type_str ;
				std::string dummy_qualifier_str ;
				bi.id = buffers.size() ;
				getDimensions(v,bi) ;
				nameToBufferId[bi.arrayName] = bi.id ;
				buffers.push_back(bi) ;

				SourceLocation ss = v->getLocStart() ;
				SourceLocation se = v->getLocEnd().getLocWithOffset(2) ;
				SourceRange SR(ss, se) ;
				TheRewriter.RemoveText(SR) ;
			}
		}
	}

	return true ;
}

//bool MyASTVisitor::VisitDecl(Decl* d)
//{
//	//if(isa<GlobalDecl>(v)) {
//		printf("Declaration = %s\n",d->getDeclKindName()) ;
//	//}
//	return true ;
//}

//printf("--- Building Def-Use Chains for function %s : %d ---\n", f->getNameAsString().c_str(), f->getNumParams()) ;
//clang::defuse::DefUse* du = defuse::DefUse::BuildDefUseChains(f, f->getBody(), &CTX, 0, 0, 0, true);
//printf("**************************************************\n") ;
//delete du; 
