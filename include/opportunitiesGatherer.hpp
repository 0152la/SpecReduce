#ifndef _OPPORTUNITIES_GATHERER_HPP
#define _OPPORTUNITIES_GATHERER_HPP

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

#include "MRGatherer.hpp"
#include "globals.hpp"

const mrInfo* checkFunctionIsMRCall(const clang::FunctionDecl*);
bool checkNameIsVariant(std::string);

class instantiatedMRVisitor : public clang::RecursiveASTVisitor<instantiatedMRVisitor>
{
    private:
        const clang::DeclRefExpr* base_dre;
        const mrInfo* fd_mri;

    public:
        instantiatedMRVisitor(clang::DeclRefExpr*, const mrInfo*);

        bool VisitDeclRefExpr(clang::DeclRefExpr*);
};

class mainTraverser : public clang::RecursiveASTVisitor<mainTraverser>
{
    private:
        std::stack<const clang::FunctionDecl*> mr_fd_stack;
        bool visited = false;

        const clang::CompoundStmt* main_child = nullptr;
        const clang::VarDecl* curr_variant_vd = nullptr;
        size_t curr_vd_index = 0;

        const clang::ast_type_traits::DynTypedNode getBaseParent(
            const clang::DeclRefExpr*);
        const clang::ast_type_traits::DynTypedNode getBaseParent(
            const clang::ast_type_traits::DynTypedNode);

    public:
        clang::ASTContext& ctx;

        mainTraverser(clang::ASTContext& _ctx) : ctx(_ctx) { };

        //bool VisitCallExpr(clang::CallExpr*);
        bool VisitVarDecl(clang::VarDecl*);
        bool VisitDeclRefExpr(clang::DeclRefExpr*);
        bool VisitCompoundStmt(clang::CompoundStmt*);

        void setVisited() { this->visited = true; };
        bool hasVisited() { return this->visited; };

        void setMainChild(const clang::CompoundStmt* cs) { this->main_child = cs; };
        const clang::Stmt* getMainChild() { return this->main_child; };
};

class mainTraverserCallback : public clang::ast_matchers::MatchFinder::MatchCallback
{
    public:
        virtual void
            run(const clang::ast_matchers::MatchFinder::MatchResult&) override;
};

/*******************************************************************************
 * Fuzzing reduction helper functions
 ******************************************************************************/

class reductionFuncParamFinder :
    public clang::RecursiveASTVisitor<reductionFuncParamFinder>
{
    public:
        std::map<std::string, std::string> concrete_param_vars;
        std::set<std::string> remaining_param_types;

        reductionFuncParamFinder(const clang::FunctionDecl*);

        bool VisitVarDecl(clang::VarDecl*);
};

// Checks whether a Stmt was previously fuzz reduced
class prevFuzzedChecker :
    public clang::RecursiveASTVisitor<prevFuzzedChecker>
{
    public:
        bool was_fuzzed = false;

        prevFuzzedChecker(const clang::Stmt*);

        bool VisitCallExpr(clang::CallExpr*);
};

class literalFinder : public clang::ast_matchers::MatchFinder::MatchCallback
{
    public:
        std::string parsed_string = "";

        virtual void
            run(const clang::ast_matchers::MatchFinder::MatchResult&) override;
};

class fuzzerGathererCallback : public clang::ast_matchers::MatchFinder::MatchCallback
{
    public:
        virtual void
            run(const clang::ast_matchers::MatchFinder::MatchResult&) override;

    private:
        std::vector<const clang::Stmt*> getFuzzingStmts(const clang::CallExpr*, const clang::CompoundStmt*);
};

class fuzzReductionFunctionGatherer : public clang::ast_matchers::MatchFinder::MatchCallback
{
    public:
        std::map<clang::QualType, const clang::FunctionDecl*> reduction_fns;
        //std::vector<const clang::FunctionDecl*> reduction_fns;

        virtual void
            run(const clang::ast_matchers::MatchFinder::MatchResult&) override;
};

class opportunitiesGatherer
{
    private:
        clang::ast_matchers::MatchFinder mr_matcher;
        mainTraverserCallback main_traverser_cb;
        fuzzerGathererCallback fuzzer_gatherer_cb;
        fuzzReductionFunctionGatherer fuzz_reduction_fns_cb;

    public:
        opportunitiesGatherer(clang::ASTContext&);
};

#endif
