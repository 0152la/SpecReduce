#ifndef _REDUCTION_PASS_HPP
#define _REDUCTION_PASS_HPP

#include "clang/AST/Decl.h"
#include "clang/AST/Expr.h"
#include "clang/AST/Stmt.h"
#include "clang/Basic/SourceLocation.h"
#include "clang/Rewrite/Core/Rewriter.h"

#include <numeric>
#include <iostream>
#include <sstream>
#include <vector>

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
            const clang::DeclRefExpr*, std::vector<reduction_data_t*>&);
};

class functionCleaner : public reductionPass
{
    public:
        functionCleaner(const clang::FunctionDecl*);
};

class variantReducer : public reductionPass
{
    public:
        variantReducer(variant_decl_t*, std::vector<reduction_data_t*>&);
};

class instructionReducer : public reductionPass
{
    public:
        instructionReducer(variant_instruction_t*, std::vector<reduction_data_t*>&);
};

class recursionReducer : public reductionPass
{
    public:
        recursionReducer(const clang::DeclRefExpr*, std::vector<reduction_data_t*>&);
};

class fuzzingInstrReducer : public reductionPass
{
    public:
        fuzzingInstrReducer(std::pair<std::string, const clang::Stmt*>,
            std::vector<reduction_data_t*>&);
};


#endif
