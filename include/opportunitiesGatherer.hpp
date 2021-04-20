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
        clang::FunctionDecl* base_fd;
        const mrInfo* fd_mri;

    public:
        instantiatedMRVisitor(clang::FunctionDecl*, const mrInfo*);

        bool VisitDeclRefExpr(clang::DeclRefExpr*);
};

class mainTraverser : public clang::RecursiveASTVisitor<mainTraverser>
{
    private:
        std::stack<const clang::FunctionDecl*> mr_fd_stack;
        clang::ASTContext& ctx;
        bool visited = false;

        const clang::CompoundStmt* main_child = nullptr;
        const clang::VarDecl* curr_variant_vd = nullptr;

        const clang::ast_type_traits::DynTypedNode getBaseParent(
            const clang::DeclRefExpr*);
        const clang::ast_type_traits::DynTypedNode getBaseParent(
            const clang::ast_type_traits::DynTypedNode);

    public:
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

class opportunitiesGatherer
{
    private:
        clang::ast_matchers::MatchFinder mr_matcher;
        mainTraverserCallback main_traverser_cb;

    public:
        opportunitiesGatherer(clang::ASTContext&);
};

#endif
