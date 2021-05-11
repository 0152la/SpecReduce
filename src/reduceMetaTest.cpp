#include <set>
#include <map>
#include <iostream>
#include <vector>
#include <tuple>

#include "clang/AST/Expr.h"
#include "clang/AST/Decl.h"
#include "clang/ASTMatchers/ASTMatchers.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "clang/Basic/SourceLocation.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Tooling/Tooling.h"
#include "clang/Tooling/CommonOptionsParser.h"
#include "clang/Tooling/Refactoring/AtomicChange.h"
#include "clang/Tooling/Refactoring/RefactoringAction.h"
#include "clang/Tooling/Refactoring/RefactoringActionRules.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/CachePruning.h"

#include "globals.hpp"
#include "reductionEngine.hpp"
#include "compileAndExecute.hpp"

static llvm::cl::OptionCategory reduceMetaTest("reduce-meta-test");
static llvm::cl::opt<size_t> ReducerSeed("seed",
    llvm::cl::desc("Seed to use in reducer"), llvm::cl::init(42),
    llvm::cl::cat(reduceMetaTest));
static llvm::cl::alias ReducerSeedAlias("s", llvm::cl::aliasopt(ReducerSeed));
static llvm::cl::opt<std::string> TestOutput("output",
    llvm::cl::desc("Path where to emit output file."),
    llvm::cl::cat(reduceMetaTest), llvm::cl::value_desc("filename"),
    llvm::cl::init("output.cpp"));
static llvm::cl::alias TestOutputAlias("o",
    llvm::cl::desc("Path where to emit output file."),
    llvm::cl::aliasopt(TestOutput), llvm::cl::cat(reduceMetaTest));
static llvm::cl::opt<size_t> ReducerNoProgressPasses("passes",
    llvm::cl::desc("Number of passes without progress until reduction gives up"),
    llvm::cl::init(10), llvm::cl::cat(reduceMetaTest));

bool debug_info = true;
bool globals::reduction_success = false;
std::string globals::test_error_message;
std::string globals::output_file;
std::string globals::interestingness_test_path = "/home/sentenced/Documents/Internships/2018_ETH/work/spec_reduce/input/simple_test.py";
int globals::test_error_code;
std::set<mrInfo*> globals::mr_names_list;
instantiated_mrs_map_t globals::instantiated_mrs;
variant_decls_map_t globals::variant_decls;
variant_instrs_map_t globals::variant_instrs;
variant_instr_index_map_t globals::variant_instr_index;

int
main(int argc, char const **argv)
{
    clang::tooling::CommonOptionsParser op(argc, argv, reduceMetaTest);

    const size_t max_pass_count = ReducerNoProgressPasses;
    size_t curr_pass_count = 0;

    globals::output_file = TestOutput;

    //const std::string t_path =  "/home/sentenced/Documents/Internships/2018_ETH/work/spec_reduce/input/t_z3.cpp";
    //const std::string cm_path = "/home/sentenced/Documents/Internships/2018_ETH/work/spec_ast/input/spec_repo/SMT_QF_NIA/z3/runtime";
    //testExecutor te(t_path, cm_path);
    //te.compileTestCase();
    //globals::test_error_code = te.executeTestCase();
    //if (globals::test_error_code == 0)
    //{
        //std::cout << "No initial error detected in provided test case; quitting..." << std::endl;
        //exit(0);
    //}

    assert(op.getSourcePathList().size() == 1);
    std::string input_file = op.getSourcePathList().front();

    size_t reduction_success = 0;
    do
    {
        EMIT_DEBUG_INFO("Reduction success count " + std::to_string(reduction_success));
        clang::tooling::ClangTool reduceTool(op.getCompilations(),
            std::vector<std::string>({input_file}));
        reduceTool.run(clang::tooling::newFrontendActionFactory
                    <reductionEngineAction>().get());
        input_file = globals::output_file;
        reduction_success += 1;
    }
    while(globals::reduction_success);

    for (const mrInfo* mri : globals::mr_names_list)
    {
        delete(mri);
    }
}
