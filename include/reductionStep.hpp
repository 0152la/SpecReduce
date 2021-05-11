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
    public:
        std::vector<reductionPass*> opportunities;

        reductionStep(reduction_datas_t);
};

#endif
