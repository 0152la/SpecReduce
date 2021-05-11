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
        variant_instr_index_map_t variant_instr_index;

        size_t chunk_size = -1, offset = 0, reduction_idx = 0;

        template<typename T, typename U>
        std::map<T, U> selectReductions(std::map<T, U>& reductions) {
            return selectLeadReduction(reductions); }

        template<typename T, typename U>
        std::map<T, U> selectLeadReduction(std::map<T, U>& reductions)
        {
            for (std::pair<T, U> reduction_pair : reductions)
            {
                if (!checkReductionMark(reduction_pair.second))
                {
                    return std::map<T,U>({reduction_pair});
                }
            }
            return std::map<T,U>();
        }

        template<typename T> bool checkReductionMark(const T* rd) {
            return rd->marked; }
        template<typename T> bool checkReductionMark(
            const std::vector<T*>& rd_vec) {
                bool all_marked = true;
                std::for_each(rd_vec.begin(), rd_vec.end(),
                    [&all_marked](T* rd) { all_marked &= rd->marked; });
                return all_marked;
        }

    public:
        std::vector<reductionPass*> opportunities;

        reductionStep(reduction_datas_t);
        //reductionStep(variant_decls_map_t, variant_instr_index_map_t,
            //variant_instrs_map_t, instantiated_mrs_map_t);
};

#endif
