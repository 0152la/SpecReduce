#ifndef _GLOBALS_HPP
#define _GLOBALS_HPP

#include <map>
#include <set>
#include <string>
#include <vector>

class reductionStep;

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

namespace globals
{

const std::string variant_prefix = "r";
const char delimiter = '_';

extern std::string test_error_message;
extern int test_error_code;
extern std::set<std::string> mr_names_list;
extern std::map<const clang::FunctionDecl*, std::vector<const clang::DeclRefExpr*>> instantiated_mrs;
extern std::map<std::string, std::vector<clang::SourceRange&>> variant_instrs;
//extern std::map<std::string, std::vector<const clang::Stmt*>> variant_instrs;
extern std::vector<reductionStep*> opportunities;
//extern std::map<REDUCTION_TYPE, std::vector<std::tuple<clang::SourceRange, clang::SourceRange>>> opportunities;
//extern std::vector<std::tuple<clang::SourceRange, std::string>> reductions;

} // namespace globals

#endif
