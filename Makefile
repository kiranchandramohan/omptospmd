CXX = g++
CFLAGS = -fno-rtti 

LLVM_SRC_PATH = /home/kiran/openmp_llvm/llvm/
LLVM_BUILD_PATH = /home/kiran/openmp_llvm/build

LLVM_BIN_PATH = $(LLVM_BUILD_PATH)/Debug+Asserts/bin
LLVM_LIBS=core mc
CLANG_BUILD_FLAGS = -I$(LLVM_SRC_PATH)/include -I$(LLVM_BUILD_PATH)/include -I$(LLVM_SRC_PATH)/tools/clang/include -I$(LLVM_BUILD_PATH)/tools/clang/include

CLANGLIBS = -lclangFrontendTool -lclangFrontend -lclangDriver -lclangSerialization -lclangCodeGen -lclangParse -lclangSema -lclangStaticAnalyzerFrontend -lclangStaticAnalyzerCheckers -lclangStaticAnalyzerCore -lclangAnalysis -lclangARCMigrate -lclangRewriteFrontend -lclangRewriteCore -lclangEdit -lclangAST -lclangLex -lclangBasic -lLLVMBitReader -lLLVMMCParser -lLLVMSupport -lLLVMOption -lLLVMTransformUtils

all: omptospmd

omptospmd: omptospmd.cc
	$(CXX) $(CLANG_BUILD_FLAGS) -D_DEBUG -D_GNU_SOURCE -D__STDC_CONSTANT_MACROS -D__STDC_FORMAT_MACROS -D__STDC_LIMIT_MACROS -g -fvisibility-inlines-hidden -fno-exceptions -fno-rtti -fPIC -Woverloaded-virtual -Wcast-qual -L/home/kiran/openmp_llvm/build/Debug+Asserts/lib  omptospmd.cc $(CFLAGS) -o omptospmd $(CLANGLIBS) -lz -lpthread -ltinfo -lrt -ldl -lm -lLLVMMC -lLLVMObject -lLLVMCore -lLLVMSupport

test:
	./omptospmd openmp/doitgen/test.c
	./omptospmd openmp/dotproduct/test.c
	./omptospmd openmp/edge_detect/test.c
	./omptospmd openmp/floydwarshall/test.c
	./omptospmd openmp/histo/test.c
	./omptospmd openmp/matmul/test.c
	./omptospmd openmp/regdetect/test.c

clean:
	rm -rf *.o *.ll omptospmd
