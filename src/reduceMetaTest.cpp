#include <chrono>
#include <ctime>
#include <fstream>
#include <iostream>
#include <iomanip>
#include <map>
#include <set>
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
static llvm::cl::opt<std::string> CompileScriptPath("compile-script",
    llvm::cl::desc("Path to compilation script"), llvm::cl::cat(reduceMetaTest),
    llvm::cl::Required);
static llvm::cl::opt<std::string> CMakeFilePath("cmake-path",
    llvm::cl::desc("Path to cmake file for compilation script"),
    llvm::cl::cat(reduceMetaTest), llvm::cl::Required);
static llvm::cl::opt<std::string> InterestTest("interest-test",
    llvm::cl::desc("Path to interestingness test script"),
    llvm::cl::cat(reduceMetaTest), llvm::cl::Required);
static llvm::cl::opt<bool> MonotonicReduction("monotonic",
    llvm::cl::desc("Whether reduction should not revisit previous attempted and exhausted type."),
    llvm::cl::init(true), llvm::cl::cat(reduceMetaTest));
static llvm::cl::opt<bool> KeepLastVariant("no-reduce-last-var",
    llvm::cl::desc("Whether to ensure one variant (in addition to base variant) "
    "is kept, even if it is not necessary to expose the reduced-for bug. Useful "
    "to ensure there is an oracle for the eventually reduced test."),
    llvm::cl::init(true), llvm::cl::cat(reduceMetaTest));
static llvm::cl::opt<bool> KeepChecks("no-reduce-checks",
    llvm::cl::desc("Whether to reduce checks as part of sequence shortening. "
    "Similar to variants, checks can be kept in order to ensure an oracle is "
    "kept."), llvm::cl::init(true), llvm::cl::cat(reduceMetaTest));
static llvm::cl::opt<bool> EnableLogging("enable-logging",
    llvm::cl::desc("Whether to enable internal logging"),
    llvm::cl::init(false), llvm::cl::cat(reduceMetaTest));
static llvm::cl::opt<std::string> LogOutput("log-output",
    llvm::cl::desc("If set, path to file where to emit logging data"),
    llvm::cl::init(""), llvm::cl::cat(reduceMetaTest));

// Globals definitions
size_t globals::debug_level;
size_t globals::reductions_count = 0;
bool globals::reduction_success = false;
bool globals::monotonic_reduction;
bool globals::keep_last_variant;
bool globals::keep_checks;
REDUCTION_TYPE globals::reduction_type_progress;
int globals::expected_return_code;
std::set<size_t> globals::observed_return_codes;

// Logging definitions
bool logging::enable_log;
long logging::initial_file_size;
long logging::final_file_size;
std::map<REDUCTION_TYPE, size_t> logging::reductions_attempted;
std::map<REDUCTION_TYPE, size_t> logging::reductions_applied;
std::time_t logging::time_start;
std::time_t logging::time_end;
//std::chrono::time_point<std::chrono::steady_clock> logging::time_start;
//std::chrono::time_point<std::chrono::steady_clock> logging::time_end;

// File paths
std::string globals::output_file;
std::string globals::interestingness_test_path;
std::string globals::compile_script_location;
std::string globals::cmake_file_path;

// Global reduction metadata
std::set<mrInfo*> globals::mr_names_list;
std::map<std::string, reduce_fn_data*> globals::reduce_fn_list;
std::set<std::string> globals::reduce_fn_names;
std::set<std::string> globals::reduce_fn_param_types;
std::set<const clang::FunctionDecl*> globals::checked_non_mrs;

// Reduction step metadata
instantiated_mrs_map_t globals::instantiated_mrs;
variant_decls_map_t globals::variant_decls;
variant_instrs_map_t globals::variant_instrs;
variant_instr_index_map_t globals::variant_instr_index;
fuzzing_regions_map_t globals::fuzzing_regions;

int
main(int argc, char const **argv)
{
    clang::tooling::CommonOptionsParser op(argc, argv, reduceMetaTest);

    const size_t max_pass_count = ReducerNoProgressPasses;
    size_t curr_pass_count = 0;

    globals::output_file = TestOutput;
    globals::debug_level = DebugLevel;
    logging::enable_log = EnableLogging;

    globals::compile_script_location = CompileScriptPath;
    globals::cmake_file_path = CMakeFilePath;
    globals::interestingness_test_path = InterestTest;

    globals::monotonic_reduction = MonotonicReduction;
    globals::keep_last_variant = KeepLastVariant;
    globals::keep_checks = KeepChecks;

    for (std::pair<REDUCTION_TYPE, std::string> typ_pair : reduction_type_names)
    {
        logging::reductions_attempted.insert({typ_pair.first, 0});
        logging::reductions_applied.insert({typ_pair.first, 0});
    }

    assert(op.getSourcePathList().size() == 1);
    std::string input_file = op.getSourcePathList().front();

    logging::initial_file_size = logging::getFileSize(input_file);
    logging::time_start = std::time(nullptr);

    /* Sanity check */
    EMIT_DEBUG_INFO("Running sanity check", 1);
    interestingExecutor int_exec(input_file);
    if (int_exec.runInterestingnessTest())
    {
        std::cout << "Sanity check return exit code ";
        std::cout << int_exec.getReturnCode() << std::endl;
        return 1;
    }
    globals::expected_return_code = int_exec.getReturnCode();
    EMIT_DEBUG_INFO("Set expected return code " +
        std::to_string(globals::expected_return_code) , 1);

    do
    {
        globals::reduction_success = false;
        assert(llvm::sys::fs::exists(input_file));
        EMIT_DEBUG_INFO("Reduction success count " +
            std::to_string(globals::reductions_count), 1);
        clang::tooling::ClangTool reduceTool(op.getCompilations(),
            std::vector<std::string>({input_file}));
        reduceTool.run(clang::tooling::newFrontendActionFactory
                    <reductionEngineAction>().get());
        input_file = globals::output_file;
    }
    while (globals::reduction_success);

    for (const mrInfo* mri : globals::mr_names_list)
    {
        delete(mri);
    }
    for (std::pair<std::string, reduce_fn_data*> rd_fn_pair : globals::reduce_fn_list)
    {
        delete(rd_fn_pair.second);
    }

    if (globals::reductions_count == 0)
    {
        EMIT_DEBUG_INFO("No reduction opportunity was applicable.", 1);
    }
    else
    {
        EMIT_DEBUG_INFO("Final reduced program written to " + globals::output_file, 1);

        logging::final_file_size = logging::getFileSize(globals::output_file);
    }

    logging::time_end = std::time(nullptr);

    if (logging::enable_log)
    {
        std::stringstream emit_log;
        emit_log << "STATISTICS ===" << std::endl;
        emit_log << "- initial file size --- " << logging::initial_file_size << std::endl;
        emit_log << "- final file size --- " << logging::final_file_size << std::endl;


        std::stringstream red_attempt_str, red_apply_str;
        size_t red_attempt_cnt = 0, red_apply_cnt = 0;
        for (std::pair<REDUCTION_TYPE, std::string> typ_pair : reduction_type_names)
        {
            std::string red_typ = "\t" + typ_pair.second + " == ";
            red_attempt_str << red_typ << logging::reductions_attempted.at(typ_pair.first) << std::endl;
            red_attempt_cnt += logging::reductions_attempted.at(typ_pair.first);
            red_apply_str << red_typ << logging::reductions_applied.at(typ_pair.first) << std::endl;
            red_apply_cnt += logging::reductions_applied.at(typ_pair.first);
        }
        emit_log << "- reduction attempts --- " << red_attempt_cnt << "\n" << red_attempt_str.str();
        emit_log << "- reductions applied --- " << red_apply_cnt << "\n" << red_apply_str.str();
        emit_log << "- return code expected --- " << globals::expected_return_code << std::endl;

        std::string return_codes_str;
        auto orc_it = globals::observed_return_codes.begin();
        auto orc_end = globals::observed_return_codes.end();
        std::string delim = ", ";
        while (orc_it != orc_end)
        {
            return_codes_str += std::to_string(*orc_it);
            orc_it = std::next(orc_it);
            if (orc_it != orc_end)
            {
                return_codes_str += delim;
            }
        }
        emit_log << "- return codes seen --- " << return_codes_str << std::endl;

        emit_log << "- time start  --- " << std::put_time(std::localtime(&logging::time_start), "%F %T") << std::endl;
        emit_log << "- time end --- " << std::put_time(std::localtime(&logging::time_end), "%F %T") << std::endl;
        emit_log << "- duration --- " << std::difftime(logging::time_end, logging::time_start) << " seconds" << std::endl;


        //emit_log << "- time start  --- " <<
            //std::put_time(std::localtime(
            //std::chrono::steady_clock::to_time_t(logging::time_start)), "%F %T")
            //<< std::endl;
        //emit_log << "- time end--- " <<
            //std::put_time(std::localtime(
            //std::chrono::steady_clock::to_time_t(logging::time_end)), "%F %T")
            //<< std::endl;
        //emit_log << "- duration --- " <<
            //std::chrono::seconds(logging::time_end - logging::time_start).count() << std::endl;
        emit_log << "=== END" << std::endl;
        if (!LogOutput.empty())
        {
            EMIT_DEBUG_INFO("Writing log data to " + LogOutput, 1);
            std::ofstream log_out;
            log_out.open(LogOutput, std::ios_base::out);
            log_out << emit_log.str();
            log_out.close();
        }
        else
        {
            std::cout << emit_log.str();
        }
    }
}
