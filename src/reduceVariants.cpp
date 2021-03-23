#include "reduceVariants.hpp"
#include "globals.hpp"

void
reduceVariants::HandleTranslationUnit(clang::ASTContext& ctx)
{
    
}

std::unique_ptr<clang::ASTConsumer>
reduceVariants::CreateASTConsumer(clang::CompilerInstance& ci,
    llvm::StringRef file)
{
    rw.setSourceMgr(ci.getSourceManager(), ci.getLangOpts());
    return std::make_unique<reduceVariants>(rw);
}
