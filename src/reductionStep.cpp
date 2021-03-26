#include "reductionStep.hpp"

void
reductionStep::applyReduction(clang::Rewriter& rw)
{
    this->to_modify.dump(rw.getSourceMgr());
    rw.ReplaceText(this->to_modify, new_string);
}

functionCleaner::functionCleaner(clang::FunctionDecl* fd)
{
    clang::SourceRange fd_range(fd->getBeginLoc(), fd->getEndLoc());
    this->to_modify = fd_range;
    this->new_string = "";
}

variantReducer::variantReducer(std::string variant_name, clang::Rewriter& rw)
{
    std::cout << variant_name << std::endl;
    std::vector<const clang::Stmt*> variant_stmts =
        globals::variant_instrs.at(variant_name);
    //clang::SourceRange reduction_range(variant_stmts.front()->getBeginLoc(),
        //variant_stmts.back()->getEndLoc());
    variant_stmts.front().dump(rw.getSourceMgr());
    assert(false);
    this->to_modify = variant_stmts.front();
    this->new_string = "";
}


