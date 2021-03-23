#ifndef _GLOBALS_HPP
#define _GLOBALS_HPP

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

extern std::map<REDUCTION_TYPE, std::vector<std::tuple>> opportunities;
extern std::vector<std::tuple> reductions;

} // namespace globals

#endif
