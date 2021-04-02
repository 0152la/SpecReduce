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

class reductionStep
{
    protected:
        clang::SourceRange to_modify;
        std::string new_string;
        std::vector<const clang::FunctionDecl*> cleanup_functions;

    public:
        virtual void applyReduction(clang::Rewriter&);

        const clang::FunctionDecl* addFunctionDREToClean(const clang::DeclRefExpr*);
};

class functionCleaner : public reductionStep
{
    public:
        functionCleaner(const clang::FunctionDecl*);
};

class variantReducer : public reductionStep
{
    public:
        variantReducer(const clang::VarDecl*);
};

class recursionReducer : public reductionStep
{
    public:
        recursionReducer(const clang::DeclRefExpr*);
};

#endif
