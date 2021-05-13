#include <chrono>
#include <set>
#include <map>
#include <iostream>
#include <vector>
#include <thread>
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
static llvm::cl::opt<size_t> DebugLevel("debug",
    llvm::cl::desc("Verbosity of debug messages"),
    llvm::cl::init(2), llvm::cl::cat(reduceMetaTest));

size_t globals::debug_level;
size_t globals::reductions_count = 0;
bool globals::reduction_success = false;
int globals::expected_return_code;
std::string globals::output_file;
std::string globals::interestingness_test_path = "/home/sentenced/Documents/Internships/2018_ETH/work/spec_reduce/scripts/interestingness.py";
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
    globals::debug_level = DebugLevel;

    assert(op.getSourcePathList().size() == 1);
    std::string input_file = op.getSourcePathList().front();

    /* Sanity check */
    interestingExecutor int_exec(input_file, globals::interestingness_test_path);
    if (int_exec.runInterestingnessTest())
    {
        std::cout << "Sanity check return exit code " << int_exec.getReturnCode() << std::endl;
        return 1;
    }
    globals::expected_return_code = int_exec.getReturnCode();
    EMIT_DEBUG_INFO("Set expected return code " +
        std::to_string(globals::expected_return_code) , 1);

    do
    {
        assert(llvm::sys::fs::exists(input_file));
        EMIT_DEBUG_INFO("Reduction success count " +
            std::to_string(globals::reductions_count), 1);
        clang::tooling::ClangTool reduceTool(op.getCompilations(),
            std::vector<std::string>({input_file}));
        reduceTool.run(clang::tooling::newFrontendActionFactory
                    <reductionEngineAction>().get());
        input_file = globals::output_file;
        globals::reductions_count += 1;
    }
    while (globals::reduction_success); // && globals::reductions_count < 3);

    for (const mrInfo* mri : globals::mr_names_list)
    {
        delete(mri);
    }
}
