#ifndef _GLOBALS_HPP
#define _GLOBALS_HPP


#include "clang/AST/Decl.h"
#include "clang/AST/Expr.h"

#include <map>
#include <set>
#include <string>
#include <vector>

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
    bool marked = false;

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
    bool marked = false;

    variant_decl_t(const clang::VarDecl* _vd) : variant_decl(_vd) {};
};

typedef std::map<const clang::VarDecl*, variant_decl_t*>
    variant_decls_map_t;
typedef std::map<const clang::Stmt*, variant_instruction_t*>
    variant_instrs_map_t;
typedef std::map<size_t, std::vector<variant_instruction_t*>>
    variant_instr_index_map_t;
typedef std::map<const clang::DeclRefExpr*, instantiated_mr_t*>
    instantiated_mrs_map_t;

struct reduction_datas_t
{
    std::vector<const clang::VarDecl*> variant_decls;
    std::vector<size_t> variant_instr_index;
    std::vector<const clang::DeclRefExpr*> recursive_calls;

    reduction_datas_t(const variant_decls_map_t& _vds,
        const variant_instr_index_map_t& _viidx,
        const instantiated_mrs_map_t& _imrs)
    {
        this->variant_decls = this->reduceMapKeysToVector(_vds);
        this->variant_instr_index = this->reduceMapKeysToVector(_viidx);
        this->recursive_calls = this->reduceMapKeysToVector(_imrs);
    }

    reduction_datas_t(std::vector<const clang::VarDecl*> _vds,
        std::vector<size_t> _viidx, std::vector<const clang::DeclRefExpr*> _rcs):
        variant_decls(_vds), variant_instr_index(_viidx), recursive_calls(_rcs) {};

    size_t getReductionsSizeByType(REDUCTION_TYPE rd_type)
    {
        switch (rd_type)
        {
            case VARIANT_ELIMINATION:
                return this->variant_decls.size();
            case FAMILY_SHORTENING:
                return this->variant_instr_index.size();
            case RECURSION_REMOVAL:
                return this->recursive_calls.size();
            default:
                assert(false);
        }
    }

    bool
    empty() const
    {
        return this->variant_decls.empty() &&
            this->variant_instr_index.empty() &&
            this->recursive_calls.empty();
    }

    private:
        template<typename T, typename U>
        std::vector<T>
        reduceMapKeysToVector(const std::map<T,U> input_map)
        {
            std::vector<T> output_vec;
            for (const std::pair<T, U> map_pair : input_map)
            {
                output_vec.push_back(map_pair.first);
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
extern int test_error_code;
extern bool reduction_success;

extern instantiated_mrs_map_t instantiated_mrs;
extern variant_decls_map_t variant_decls;
extern variant_instrs_map_t variant_instrs;
extern variant_instr_index_map_t variant_instr_index;
extern std::set<mrInfo*> mr_names_list;

} // namespace globals

#endif
