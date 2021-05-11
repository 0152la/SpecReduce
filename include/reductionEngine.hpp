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

        template<typename T>
        std::vector<T> selectChunks(const std::vector<T>& reductions)
        {
            assert(this->offset < reductions.size());
            typename std::vector<T>::const_iterator red_begin = reductions.cbegin();
            typename std::vector<T>::const_iterator select_start_it =
                std::next(red_begin, this->offset);
            typename std::vector<T>::const_iterator select_end_it =
                this->offset + this->chunk_size >= reductions.size()
                ? reductions.cend()
                : std::next(select_start_it, this->chunk_size);

            this->offset += chunk_size;
            return std::vector<T>(select_start_it, select_end_it);
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
