#ifndef _REDUCTION_STEP_HPP
#define _REDUCTION_STEP_HPP

#include "clang/AST/Decl.h"
#include "clang/AST/Expr.h"
#include "clang/AST/Stmt.h"
#include "clang/Basic/SourceLocation.h"
#include "clang/Rewrite/Core/Rewriter.h"

#include <vector>

#include <iostream>

#include "globals.hpp"

class reductionStep
{
    protected:
        clang::SourceRange to_modify;
        std::string new_string;

    public:
        void applyReduction(clang::Rewriter&);
};

class functionCleaner : public reductionStep
{
    public:
        functionCleaner(clang::FunctionDecl*);
};

class variantReducer : public reductionStep
{
    public:
        variantReducer(std::string, clang::Rewriter&);
};

//class recursionReducer : public reductionStep
//{
    //public:
        //recursionReducer(const clang::DeclRefExpr*);

//};

#endif
