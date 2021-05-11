#ifndef _REDUCTION_PASS_HPP
#define _REDUCTION_PASS_HPP

#include "clang/AST/Decl.h"
#include "clang/AST/Expr.h"
#include "clang/AST/Stmt.h"
#include "clang/Basic/SourceLocation.h"
#include "clang/Rewrite/Core/Rewriter.h"

#include <vector>

#include <iostream>

#include "helperFunctions.hpp"
#include "globals.hpp"

class reductionPass
{
    protected:
        clang::SourceRange to_modify;
        std::string new_string;
        std::vector<const clang::FunctionDecl*> cleanup_functions;

    public:
        virtual void applyReduction(clang::Rewriter&);

        void addFunctionDREToClean(
            const clang::DeclRefExpr*, instantiated_mrs_map_t&);
};

class functionCleaner : public reductionPass
{
    public:
        functionCleaner(const clang::FunctionDecl*);
};

class variantReducer : public reductionPass
{
    public:
        variantReducer(variant_decl_t*, variant_instrs_map_t&,
            instantiated_mrs_map_t&);
};

class instructionReducer : public reductionPass
{
    public:
        instructionReducer(variant_instruction_t*,
            instantiated_mrs_map_t&);
};

class recursionReducer : public reductionPass
{
    public:
        recursionReducer(const clang::DeclRefExpr*, instantiated_mrs_map_t&);
};

#endif
