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
#include "omptospmd.h" 

using namespace clang;

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
