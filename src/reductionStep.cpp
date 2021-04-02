#include "reductionStep.hpp"

void
reductionStep::applyReduction(clang::Rewriter& rw)
{
    rw.ReplaceText(this->to_modify, new_string);
    for (const clang::FunctionDecl*  fd : this->cleanup_functions)
    {
        functionCleaner fc(fd);
        fc.applyReduction(rw);
    }
}

const clang::FunctionDecl*
reductionStep::addFunctionDREToClean(const clang::DeclRefExpr* dre)
{
    EMIT_DEBUG_INFO("Adding cleanup function " + dre->getNameInfo().getAsString());
    const clang::FunctionDecl* dre_fd =
        llvm::dyn_cast<clang::FunctionDecl>(dre->getDecl());
    assert(dre_fd);
    this->cleanup_functions.push_back(dre_fd);
    return dre_fd;
}

/*******************************************************************************
 * functionCleaner
 ******************************************************************************/

functionCleaner::functionCleaner(const clang::FunctionDecl* fd)
{
    EMIT_DEBUG_INFO("Cleaning orphaned function " + fd->getNameAsString());
    clang::SourceRange fd_range(fd->getBeginLoc(), fd->getEndLoc());
    this->to_modify = fd_range;
    this->new_string = "";
}

/*******************************************************************************
 * variantReducer
 ******************************************************************************/

variantReducer::variantReducer(const clang::VarDecl* vd)
{
    EMIT_DEBUG_INFO("Removing variant " + vd->getNameAsString());
    std::vector<std::pair<const clang::Stmt*, const clang::DeclRefExpr*>>
        variant_stmts = globals::variant_instrs.at(vd);
    clang::SourceRange reduction_range(vd->getBeginLoc(),
        variant_stmts.back().first->getEndLoc());
    for (std::pair<const clang::Stmt*, const clang::DeclRefExpr*> p :
            globals::variant_instrs.at(vd))
    {
        if (p.second)
        {
            this->addFunctionDREToClean(p.second);
        }
    }
    this->to_modify = reduction_range;
    this->new_string = "";
}

/*******************************************************************************
 * recursionReducer
 ******************************************************************************/

recursionReducer::recursionReducer(const clang::DeclRefExpr* dre)
{
    EMIT_DEBUG_INFO("Folding recursion call " + dre->getNameInfo().getAsString());
    this->addFunctionDREToClean(dre);
    this->to_modify = dre->getSourceRange();
    this->new_string = "base";
}
