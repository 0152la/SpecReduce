#ifndef _REDUCTION_STEP_HPP
#define _REDUCTION_STEP_HPP

#include "clang/AST/Decl.h"
#include "clang/AST/Expr.h"
#include "clang/AST/Stmt.h"
#include "clang/Basic/SourceLocation.h"
#include "clang/Rewrite/Core/Rewriter.h"

#include <vector>

#include <iostream>

#include "helperFunctions.hpp"
#include "globals.hpp"
#include "reductionPass.hpp"

class reductionStep
{
    private:
        variant_instrs_map_t variant_instrs;
        variant_decls_map_t variant_decls;
        instantiated_mrs_map_t instantiated_mrs;

        size_t chunk_factor = 2;

        template<typename T, typename U>
        std::map<T, U> selectReductions(std::map<T, U>& reductions) {
            return selectLeadReduction(reductions); }

        template<typename T, typename U>
        std::map<T, U> selectLeadReduction(std::map<T, U>& reductions) {
            for (std::pair<T, U> reduction_pair : reductions)
            {
                if (!reduction_pair.second.marked)
                {
                    return std::map<T,U>({reduction_pair});
                }
            }
            return std::map<T,U>();
        }

    public:
        std::vector<reductionPass*> opportunities;

        reductionStep(variant_decls_map_t, variant_instrs_map_t,
            instantiated_mrs_map_t);
};

#endif
