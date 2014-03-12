CXX = g++
CFLAGS = -fno-rtti -fexceptions

LLVM_SRC_PATH = /home/kiran/openmp_llvm/llvm/
LLVM_BUILD_PATH = /home/kiran/openmp_llvm/build

LLVM_BIN_PATH = $(LLVM_BUILD_PATH)/Debug+Asserts/bin
LLVM_LIBS=core mc
CLANG_BUILD_FLAGS = -I$(LLVM_SRC_PATH)/include -I$(LLVM_BUILD_PATH)/include -I$(LLVM_SRC_PATH)/tools/clang/include -I$(LLVM_BUILD_PATH)/tools/clang/include

CLANGLIBS = -lclangFrontendTool -lclangFrontend -lclangDriver -lclangSerialization -lclangCodeGen -lclangParse -lclangSema -lclangStaticAnalyzerFrontend -lclangStaticAnalyzerCheckers -lclangStaticAnalyzerCore -lclangAnalysis -lclangARCMigrate -lclangRewriteFrontend -lclangRewriteCore -lclangEdit -lclangAST -lclangLex -lclangBasic -lLLVMBitReader -lLLVMMCParser -lLLVMSupport -lLLVMOption -lLLVMTransformUtils

all: omptospmd


DefUse.o : DefUse.cc
	$(CXX) $(CLANG_BUILD_FLAGS) -D_DEBUG -D_GNU_SOURCE -D__STDC_CONSTANT_MACROS -D__STDC_FORMAT_MACROS -D__STDC_LIMIT_MACROS -g -fvisibility-inlines-hidden -fno-exceptions -fno-rtti -fPIC -Woverloaded-virtual -Wcast-qual -L/home/kiran/openmp_llvm/build/Debug+Asserts/lib  DefUse.cc $(CFLAGS) -c $(CLANGLIBS) -lz -lpthread -ltinfo -lrt -ldl -lm -lLLVMMC -lLLVMObject -lLLVMCore -lLLVMSupport

omptospmd.o : omptospmd.cc omptospmd.h
	$(CXX) $(CLANG_BUILD_FLAGS) -D_DEBUG -D_GNU_SOURCE -D__STDC_CONSTANT_MACROS -D__STDC_FORMAT_MACROS -D__STDC_LIMIT_MACROS -g -fvisibility-inlines-hidden -fno-exceptions -fno-rtti -fPIC -Woverloaded-virtual -Wcast-qual -L/home/kiran/openmp_llvm/build/Debug+Asserts/lib  omptospmd.cc $(CFLAGS) -c $(CLANGLIBS) -lz -lpthread -ltinfo -lrt -ldl -lm -lLLVMMC -lLLVMObject -lLLVMCore -lLLVMSupport

main.o : main.cc omptospmd.h
	$(CXX) $(CLANG_BUILD_FLAGS) -D_DEBUG -D_GNU_SOURCE -D__STDC_CONSTANT_MACROS -D__STDC_FORMAT_MACROS -D__STDC_LIMIT_MACROS -g -fvisibility-inlines-hidden -fno-exceptions -fno-rtti -fPIC -Woverloaded-virtual -Wcast-qual -L/home/kiran/openmp_llvm/build/Debug+Asserts/lib  main.cc $(CFLAGS) -c  $(CLANGLIBS) -lz -lpthread -ltinfo -lrt -ldl -lm -lLLVMMC -lLLVMObject -lLLVMCore -lLLVMSupport

omptospmd : main.o omptospmd.o DefUse.o
	$(CXX) $(CLANG_BUILD_FLAGS) -D_DEBUG -D_GNU_SOURCE -D__STDC_CONSTANT_MACROS -D__STDC_FORMAT_MACROS -D__STDC_LIMIT_MACROS -g -fvisibility-inlines-hidden -fno-exceptions -fno-rtti -fPIC -Woverloaded-virtual -Wcast-qual -L/home/kiran/openmp_llvm/build/Debug+Asserts/lib  main.o omptospmd.o DefUse.o $(CFLAGS) -o omptospmd  $(CLANGLIBS) -lz -lpthread -ltinfo -lrt -ldl -lm -lLLVMMC -lLLVMObject -lLLVMCore -lLLVMSupport


runtest: dgentest dotptest edgetest flwltest histtest mxmtest regdtest

run: dgen dotp edge flwl hist mxm regd

dgentest:
	-./omptospmd openmp_ptr/doitgen/test.c > test_ptr/doitgen.c && clang -c test_ptr/doitgen.c
	-./omptospmd openmp_array/doitgen/test.c > test_array/doitgen.c && clang -c test_array/doitgen.c

dotptest:
	-./omptospmd openmp_ptr/dotproduct/test.c > test_ptr/dotproduct.c && clang -c test_ptr/dotproduct.c
	-./omptospmd openmp_array/dotproduct/test.c > test_array/dotproduct.c && clang -c test_array/dotproduct.c

edgetest:
	-./omptospmd openmp_ptr/edge_detect/test.c > test_ptr/edge_detect.c && clang -c test_ptr/edge_detect.c
	-./omptospmd openmp_array/edge_detect/test.c > test_array/edge_detect.c && clang -c test_array/edge_detect.c

flwltest:
	-./omptospmd openmp_ptr/floydwarshall/test.c > test_ptr/floydwarshall.c && clang -c test_ptr/floydwarshall.c
	-./omptospmd openmp_array/floydwarshall/test.c > test_array/floydwarshall.c && clang -c test_array/floydwarshall.c

histtest:
	-./omptospmd openmp_ptr/histo/test.c > test_ptr/histo.c && clang -c test_ptr/histo.c
	-./omptospmd openmp_array/histo/test.c > test_array/histo.c && clang -c test_array/histo.c

mxmtest:
	-./omptospmd openmp_ptr/matmul/test.c > test_ptr/matmul.c && clang -c test_ptr/matmul.c
	#-./omptospmd openmp_array/matmul/test.c > test_array/matmul.c && clang -c test_array/matmul.c

dgen:
	-./omptospmd openmp_ptr/doitgen/test.c && gcc -c output/test_a9.c && clang -c output/test_a9.c
	-./omptospmd openmp_array/doitgen/test.c && gcc -c output/test_a9.c && clang -c output/test_a9.c

dotp:
	-./omptospmd openmp_ptr/dotproduct/test.c && gcc -c output/test_a9.c && clang -c output/test_a9.c
	-./omptospmd openmp_array/dotproduct/test.c && gcc -c output/test_a9.c && clang -c output/test_a9.c

edge:
	-./omptospmd openmp_ptr/edge_detect/test.c && gcc -c output/test_a9.c && clang -c output/test_a9.c
	-./omptospmd openmp_array/edge_detect/test.c && gcc -c output/test_a9.c && clang -c output/test_a9.c

flwl:
	-./omptospmd openmp_ptr/floydwarshall/test.c && gcc -c output/test_a9.c && clang -c output/test_a9.c
	-./omptospmd openmp_array/floydwarshall/test.c && gcc -c output/test_a9.c && clang -c output/test_a9.c

hist:
	-./omptospmd openmp_ptr/histo/test.c && gcc -c output/test_a9.c && clang -c output/test_a9.c
	-./omptospmd openmp_array/histo/test.c && gcc -c output/test_a9.c && clang -c output/test_a9.c

mxm:
	-./omptospmd openmp_ptr/matmul/test.c && gcc -c output/test_a9.c && clang -c output/test_a9.c
	-./omptospmd openmp_array/matmul/test.c && gcc -c output/test_a9.c && clang -c output/test_a9.c

regd:
	-./omptospmd openmp_ptr/regdetect/test.c && gcc -c output/test_a9.c && clang -c test_ptr/regdetect.c
	-./omptospmd openmp_array/regdetect/test.c && gcc -c output/test_a9.c && clang -c test_array/regdetect.c

clean:
	rm -rf *.o *.ll omptospmd
	rm -f test_ptr/doitgen.c test_array/doitgen.c
	rm -f test_ptr/dotproduct.c test_array/dotproduct.c
	rm -f test_ptr/edge_detect.c test_array/edge_detect.c
	rm -f test_ptr/floydwarshall.c test_array/floydwarshall.c
	rm -f test_ptr/histo.c test_array/histo.c
	rm -f test_ptr/matmul.c test_array/matmul.c 
	rm -f test_ptr/regdetect.c test_array/regdetect.c
