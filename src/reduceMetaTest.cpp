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

static llvm::cl::OptionCategory reduceMetaTest("reduce-meta-test");
static llvm::cl::opt<size_t> ReducerSeed("seed",
    llvm::cl::desc("Seed to use in reducer"), llvm::cl::init(42),
    llvm::cl::cat(reduceMetaTest));
static llvm::cl::alias ReducerSeedAlias("s", llvm::cl::aliasopt(ReducerSeed));
static llvm::cl::opt<std::string> TestOutput("output",
    llvm::cl::desc("Path where to emit output file."),
    llvm::cl::cat(reduceMetaTest), llvm::cl::Required, llvm::cl::value_desc("filename"));
static llvm::cl::alias TestOutputAlias("o",
    llvm::cl::desc("Path where to emit output file."),
    llvm::cl::aliasopt(TestOutput), llvm::cl::cat(reduceMetaTest));
static llvm::cl::opt<size_t> ReducerNoProgressPasses("passes",
    llvm::cl::desc("Number of passes without progress until reduction gives up"),
    llvm::cl::init(10), llvm::cl::cat(reduceMetaTest));

bool debug_info = true;

void
EMIT_DEBUG_INFO(const std::string& debug_message)
{
    if (debug_info)
    {
        std::cout << "DEBUG]  " << debug_message << std::endl;
    }
}

std::map<REDUCTION_TYPE, std::vector<std::tuple>>
selectReductions(const std::map<REDUCTION_TYPE, std::vector<std::tuple>>& opportunities)
{
    return opportunities;
}

int
main(int argc, char const **argv)
{
    clang::tooling::CommonOptionsParser op(arg, argv, reduceMetaTest);

    const size_t max_pass_count = ReducerNoProgressPasses;
    size_t curr_pass_count = 0;

    assert(op.getSourcePathList().size() == 1);
    std::string input_file = op.getSourcePathList().front();
    clang::tooling::ClangTool reduceTool(op.getCompilations(),
        op.getSourcePathList());

    std::map<REDUCTION_TYPE, std::vector<std::tuple>> reductions;
    std::vector<std::pair<REDUCTION_TYPE,
                        std::unique_ptr<clang::tooling::FrontendActionFactory>>
        reduction_passes =
            {
                { VARIANT_ELIMINATION, clang::tooling::newFrontendActionFactory<reduceVariants>() },
                { FAMILY_SHORTENING  , clang::tooling::newFrontendActionFactory<reduceFamilySequence>()   },
                { RECURSION_REMOVAL  , clang::tooling::newFrontendActionFactory<reduceRecursion>()   },
                { FUZZING_REDUCTION  , clang::tooling::newFrontendActionFactory<reduceFuzzing>()   },
            };

    do
    {
        if (reduceTool.run(clang::tooling::NewFrontendActionFactory<gatherReductionOpportunities>().get()))
        {
            std::cout << "Error in gathering reduction opportunities." << std::endl;
            exit(1);
        }
        if (opportunities.empty())
        {
            break;
        }
        std::map<REDUCTION_TYPE, std::vector<std::tuple>> selected_reductions =
            selectReductions(opportunities);
        for (std::pair<REDUCTION_TYPE,
                std::unique_ptr<clang::tooling::FrontendActionFactory> r :
                    reduction_passes)
        {
            if (selected_reductions.count(r.first))
            {
                if (reduceTool.run(clang::tooling::NewFrontendActionFactory<reduceVariants>().get()))
                {
                    std::cout << "Error in reduction action VARIANT_ELIMINATION" << std::endl
                    exit(1);
                }
            }
        }
        if (!reductions.empty)
        {
            curr_pass_count = 0;
        }
        else
        {
            curr_pass_count += 1;
        }
    }
    while (!opportunities.empty() && curr_pass_count < max_pass_count);


}
