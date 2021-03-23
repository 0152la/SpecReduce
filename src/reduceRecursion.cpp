#include "reduceRecursion.hpp"
#include "globals.hpp"

void
reduceRecursion::HandleTranslationUnit(clang::ASTContext& ctx)
{
    
}

std::unique_ptr<clang::ASTConsumer>
reduceRecursion::CreateASTConsumer(clang::CompilerInstance& ci,
    llvm::StringRef file)
{
    rw.setSourceMgr(ci.getSourceManager(), ci.getLangOpts());
    return std::make_unique<reduceRecursion>(rw);
}
