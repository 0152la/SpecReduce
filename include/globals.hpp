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


struct instantiated_mr_t
{
    const clang::FunctionDecl* mr_decl;
    std::vector<const clang::DeclRefExpr*> recursive_calls;
    bool marked = false;
    const mrInfo* mr_info;

    instantiated_mr_t(const clang::FunctionDecl* _fd, const mrInfo* _mri) :
        mr_decl(_fd), mr_info(_mri) {};
};

struct variant_instruction_t
{
    const clang::Stmt* instr;
    const clang::VarDecl* assoc_variant;
    std::vector<const clang::DeclRefExpr*> mr_calls;
    bool marked = false;

    variant_instruction_t(const clang::Stmt* _s, const clang::VarDecl* _vd) :
        instr(_s), assoc_variant(_vd) {};
};

struct variant_decl_t
{
    const clang::VarDecl* variant_decl;
    std::vector<const clang::Stmt*> instrs;
    bool marked = false;

    variant_decl_t(const clang::VarDecl* _vd) : variant_decl(_vd) {};
};

typedef std::map<const clang::VarDecl*, variant_decl_t>
    variant_decls_map_t;
typedef std::map<const clang::Stmt*, variant_instruction_t>
    variant_instrs_map_t;
typedef std::map<const clang::FunctionDecl*, instantiated_mr_t>
    instantiated_mrs_map_t;

//struct variantInfo
//{
    //const clang::VarDecl* vd;
    //std::vector<std::pair<const clang::Stmt*, std::vector<const clang::DeclRefExpr*>>> stmts;
//};

//struct instantiatedMRInfo
//{
    //const clang::FunctionDecl* fd;
    //std::vector<const clang::DeclRefExpr*>

//};

namespace globals
{

const std::string variant_prefix = "r";
const char delimiter = '_';

extern std::string output_file;
extern std::string test_error_message;
extern int test_error_code;

extern instantiated_mrs_map_t instantiated_mrs;
extern variant_decls_map_t variant_decls;
extern variant_instrs_map_t variant_instrs;
extern std::set<mrInfo*> mr_names_list;

} // namespace globals

#endif
