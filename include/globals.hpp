#ifndef _GLOBALS_HPP
#define _GLOBALS_HPP


#include "clang/AST/Decl.h"
#include "clang/AST/Expr.h"

#include <map>
#include <set>
#include <string>
#include <vector>

class reductionStep;

typedef std::map<const clang::VarDecl*,
    std::vector<std::pair<const clang::Stmt*, const clang::DeclRefExpr*>>>
        variant_instrs_t;
typedef std::pair<const clang::VarDecl*,
    std::vector<std::pair<const clang::Stmt*, const clang::DeclRefExpr*>>>
        variant_instrs_elem_t;
typedef std::map<const clang::FunctionDecl*,
    std::vector<const clang::DeclRefExpr*>> instantiated_mrs_t;
typedef std::pair<const clang::FunctionDecl*,
    std::vector<const clang::DeclRefExpr*>> instantiated_mrs_elem_t;

enum REDUCTION_TYPE
{
    VARIANT_ELIMINATION,
    FAMILY_SHORTENING,
    RECURSION_REMOVAL,
    FUZZING_REDUCTION,
};

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

const std::map<REDUCTION_TYPE, std::string> reduction_type_names
    {
        { VARIANT_ELIMINATION, "VARIANT_ELIMINATION" },
        { FAMILY_SHORTENING  , "FAMILY_SHORTENING"   },
        { RECURSION_REMOVAL  , "RECURSION_REMOVAL"   },
        { FUZZING_REDUCTION  , "FUZZING_REDUCTION"   },
    };

namespace globals
{

const std::string variant_prefix = "r";
const char delimiter = '_';

extern std::string output_file;
extern std::string test_error_message;
extern int test_error_code;
extern std::set<std::string> mr_names_list;
extern instantiated_mrs_t instantiated_mrs;
extern variant_instrs_t variant_instrs;
//extern std::map<std::string, std::vector<const clang::Stmt*>> variant_instrs;
extern std::vector<reductionStep*> opportunities;
//extern std::map<REDUCTION_TYPE, std::vector<std::tuple<clang::SourceRange, clang::SourceRange>>> opportunities;
//extern std::vector<std::tuple<clang::SourceRange, std::string>> reductions;

} // namespace globals

#endif
