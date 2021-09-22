#ifndef _GLOBALS_HPP
#define _GLOBALS_HPP

#include "clang/AST/Decl.h"
#include "clang/AST/Expr.h"

#include <map>
#include <set>
#include <string>
#include <vector>

#include "logging.hpp"
#include "fuzzingReduction.hpp"

struct mrInfo;

enum REDUCTION_TYPE
{
    VARIANT_ELIMINATION,
    FAMILY_SHORTENING,
    RECURSION_REMOVAL,
    FUZZING_REDUCTION,
};

const std::map<REDUCTION_TYPE, std::string> reduction_type_names
    {
        { VARIANT_ELIMINATION, "VARIANT_ELIMINATION" },
        { FAMILY_SHORTENING  , "FAMILY_SHORTENING"   },
        { RECURSION_REMOVAL  , "RECURSION_REMOVAL"   },
        { FUZZING_REDUCTION  , "FUZZING_REDUCTION"   },
    };

struct reduction_data_t
{
    bool marked = false;
    reduction_data_t() {};
};

/**
 * Handler for instantiated MRs within the test case
 *
 * \param mr_decl { Reference to the Clang FunctionDecl representing the instantiated MR definition }
 * \param calling_dre { The Clang DeclRefExpr part of the CallExpr used to call the instantiated MR. By construction, we expect this to be unique for each respective FunctionDecl }
 * \param recursive_calls { Any further calls to other instantiated MRs contained within this one }
 * \param marked { If set, this was reduced by another operation, and should not be attempted to be reduced further }
 * \param mr_info { Internal representation of the original abstract MR }
 */

struct instantiated_mr_t : reduction_data_t
{
    const clang::FunctionDecl* mr_decl;
    const clang::DeclRefExpr* calling_dre;
    std::vector<const clang::DeclRefExpr*> recursive_calls;
    const mrInfo* mr_info;

    instantiated_mr_t(const clang::DeclRefExpr* _dre, const mrInfo* _mri) :
        calling_dre(_dre), mr_info(_mri),
        mr_decl(llvm::dyn_cast<clang::FunctionDecl>(_dre->getDecl()))
        {
            assert(this->mr_decl != nullptr);
        };
};

/**
 * Handler for individual instructions part of the variant family chain
 *
 * \param instr { Reference to the Clang Stmt representing the instruction }
 * \param assoc_variant { The associated variant this instruction acts upon }
 * \param mr_calls { Any calls to instantiated MRs contained within this instruction }
 * \param marked { If set, this was reduced by another operation, and should not be attempted to be reduced further }
 */

struct variant_instruction_t : reduction_data_t
{
    const clang::Stmt* instr;
    const clang::VarDecl* assoc_variant;
    std::vector<const clang::DeclRefExpr*> mr_calls;

    variant_instruction_t(const clang::Stmt* _s, const clang::VarDecl* _vd) :
        instr(_s), assoc_variant(_vd) {};
};

/**
 * Handler for metamorphic variants
 *
 * \param variant_decl { Reference to the Clang VarDecl representing the variable used for this variant }
 * \param instrs { All instructions (or the entire concrete family chain) corresponding to this variant }
 * \param marked { If set, this was reduced by another operation, and should not be attempted to be reduced further }
 */

struct variant_decl_t : reduction_data_t
{
    const clang::VarDecl* variant_decl;
    std::vector<const clang::Stmt*> instrs;

    variant_decl_t(const clang::VarDecl* _vd) : variant_decl(_vd) {};
};

/**
 * Handler for fuzzing regions
 *
 * \param identifier { The final variable name that is fuzzed by this region }
 * \param instrs { The list of instructions within the region, in reverse order }
 */

struct fuzzing_region_t : reduction_data_t
{
    const std::string identifier = "";
    //fuzzingTree* fuzzing_tree;
    std::vector<const clang::Stmt*> instrs;
    std::map<std::string, std::string> param_names;

    //fuzzing_region_t(std::vector<const clang::Stmt*> _instrs,
        //clang::ASTContext& ctx) {
            //this->fuzzing_tree = new fuzzingTree(_instrs, ctx); };
    //
    fuzzing_region_t(std::string _iden, std::vector<const clang::Stmt*> _instrs,
        std::map<std::string, std::string> _params, clang::ASTContext& ctx) :
            identifier(_iden), instrs(_instrs), param_names(_params) {};
};

struct reduce_fn_data
{
    //const clang::FunctionDecl* reduce_fn;
    std::string fn_name;
    std::vector<std::string> reduce_fn_arg_types;

    reduce_fn_data(const clang::FunctionDecl*);
};

typedef std::map<const clang::VarDecl*, variant_decl_t*>
    variant_decls_map_t;
typedef std::map<const clang::Stmt*, variant_instruction_t*>
    variant_instrs_map_t;
typedef std::map<size_t, std::vector<variant_instruction_t*>>
    variant_instr_index_map_t;
typedef std::map<const clang::DeclRefExpr*, instantiated_mr_t*>
    instantiated_mrs_map_t;
typedef std::map<std::string, fuzzing_region_t*>
    fuzzing_regions_map_t;

struct reduction_datas_t
{
    std::vector<const clang::VarDecl*> variant_decls;
    std::vector<size_t> variant_instr_index;
    std::vector<const clang::DeclRefExpr*> recursive_calls;
    std::vector<std::pair<std::string, const clang::Stmt*>> fuzzed_instrs;

    reduction_datas_t(const variant_decls_map_t,
        const variant_instr_index_map_t, const instantiated_mrs_map_t,
        const fuzzing_regions_map_t);

    reduction_datas_t(std::vector<const clang::VarDecl*> _vds,
        std::vector<size_t> _viidx, std::vector<const clang::DeclRefExpr*> _rcs,
        std::vector<std::pair<std::string, const clang::Stmt*>> _fr):
        variant_decls(_vds), variant_instr_index(_viidx), recursive_calls(_rcs),
        fuzzed_instrs(_fr) {};

    size_t getReductionsSizeByType(REDUCTION_TYPE) const;
    bool empty() const;

    private:
        std::vector<std::pair<std::string, const clang::Stmt*>>
        reduceFuzzingRegionsToInstr(
            const std::map<std::string, fuzzing_region_t*>);

        template<typename T, typename U>
        std::vector<T>
        reduceMapKeysToVector(const std::map<T,U>& input_map)
        {
            std::vector<T> output_vec;
            for (const std::pair<T, U> map_pair : input_map)
            {
                output_vec.push_back(map_pair.first);
            }
            return output_vec;
        }

        template<typename T, typename U>
        std::vector<T>
        reduceMapValuesToVector(const std::map<T,U>& input_map)
        {
            std::vector<U> output_vec;
            for (const std::pair<T, U> map_pair : input_map)
            {
                output_vec.push_back(map_pair.second);
            }
            return output_vec;
        }
};

namespace globals
{

const std::string variant_prefix = "r";
const char delimiter = '_';

extern std::string output_file;
extern std::string test_error_message;
extern std::string interestingness_test_path;
extern std::string compile_script_location;
extern std::string cmake_file_path;

// Reduction parameters
extern size_t debug_level;
extern bool keep_last_variant;
extern bool keep_checks;
extern std::set<size_t> observed_return_codes;

// Individual reduction attempt properties
extern int expected_return_code;
extern size_t reductions_count;
extern bool reduction_success;
extern bool monotonic_reduction;
extern REDUCTION_TYPE reduction_type_progress;

extern instantiated_mrs_map_t instantiated_mrs;
extern variant_decls_map_t variant_decls;
extern variant_instrs_map_t variant_instrs;
extern variant_instr_index_map_t variant_instr_index;
extern fuzzing_regions_map_t fuzzing_regions;

extern std::set<mrInfo*> mr_names_list;
extern std::map<std::string, reduce_fn_data*> reduce_fn_list;
extern std::set<std::string> reduce_fn_names;
extern std::set<std::string> reduce_fn_param_types;
extern std::set<const clang::FunctionDecl*> checked_non_mrs;

} // namespace globals

#endif
