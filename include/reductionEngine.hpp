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

#include "reductionStep.hpp"
#include "opportunitiesGatherer.hpp"

class reductionEngine : public clang::ASTConsumer
{
    private:
        clang::Rewriter& rw;

    public:
        reductionEngine(clang::Rewriter& _rw) : rw(_rw) {};

        void HandleTranslationUnit(clang::ASTContext&) override;
};

class reductionEngineAction : public clang::ASTFrontendAction
{
    public:
        reductionEngineAction() {};

        std::unique_ptr<clang::ASTConsumer>
            CreateASTConsumer(clang::CompilerInstance&, llvm::StringRef)
            override;
};

#endif
