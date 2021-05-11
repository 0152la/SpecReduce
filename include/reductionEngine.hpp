#ifndef _REDUCTION_ENGINE_HPP
#define _REDUCTION_ENGINE_HPP

#include <iostream>
#include <map>
#include <regex>
#include <set>
#include <stack>

#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/ASTMatchers/ASTMatchers.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Frontend/FrontendAction.h"
#include "clang/Rewrite/Core/Rewriter.h"
#include "llvm/Support/FileSystem.h"

#include "helperFunctions.hpp"

#include "MRGatherer.hpp"
#include "reductionStep.hpp"
#include "opportunitiesGatherer.hpp"

struct reduction_datas_t
{
    std::vector<const clang::VarDecl*> variant_decls;
    std::vector<size_t> variant_instr_index;
    std::vector<const clang::DeclRefExpr*> recursive_calls;

    reduction_datas_t(variant_decls_map_t, variant_instr_index_map_t,
        instantiated_mrs_map_t)

    size_t getReductionsSizeByType(REDUCTION_TYPE);

    private:
        template<typename T, typename U>
        std::vector<T>
        reduceMapKeysToVector(std::map<T,U> input_map)
        {
            std::vector<T> output_vec;
            for (std::pair<T, U> map_pair : input_map)
            {
                output_vec.push_back(map_pair.first);
            }
            return output_vec;
        }
};

class reductionEngine : public clang::ASTConsumer
{
    private:
        clang::Rewriter& rw;
        size_t chunk_size = -1, offset = 0;
        const size_t max_reduction_attempts = 3;
        REDUCTION_TYPE rd_type = VARIANT_ELIMINATION;

        reduction_datas_t selectReductions(const reduction_datas_t& rds) {
            return selectSequentialChunkReductions(rds); }
        reduction_datas_t selectSequentialChunkReductions(const reduction_datas_t&);

        template<typename T, typename U>
        std::map<T, U> selectChunks(const std::map<T, U>& reductions)
        {
            auto begin_it = reductions.begin();
            std::advance(begin_it, this->offset);

            size_t end_offset = this->offset + this->chunk_size;
            end_offset = end_offset > reductions.size() ? reductions.size() : end_offset;
            auto end_it = reductions.begin();
            //std::cout << typeid(end_it) << std::endl;
            std::advance(begin_it, end_offset);

            return std::map<T, U>(begin_it, end_it);
        }

        void cleanup();

    public:

        reductionEngine(clang::Rewriter& _rw) : rw(_rw) {};

        void HandleTranslationUnit(clang::ASTContext&) override;
};

class reductionEngineAction : public clang::ASTFrontendAction
{
    private:
        clang::Rewriter rw;

    public:
        reductionEngineAction() {};

        void EndSourceFileAction() override;

        std::unique_ptr<clang::ASTConsumer>
            CreateASTConsumer(clang::CompilerInstance&, llvm::StringRef)
            override;
};

#endif
