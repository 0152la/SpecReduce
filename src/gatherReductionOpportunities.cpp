#include "gatherReductionOpportunities.hpp"
#include "globals.hpp"

bool
checkFunctionIsMRCall(const clang::FunctionDecl* fd)
{
    std::string fd_name = fd.getNameAsString();
    std::string::iterator r_it = fd_name.rend();
    size_t delim_count = 0, expected_count = 2;
    char delim = '_';
    while (delim_count < expected_count)
    {
        if (*r_it == delim)
        {
            delim_count += 1;
        std::next(r_it);
    }
    fd_name = std::string(fd_name.begin(), r_it);
    return mr_names.contains(fd_name);
}

bool
mainTraverser::TraverseDecl(clang::Decl* d)
{
    clang::RecursiveASTVisitor<mainTraverser>::TraverseDecl(d);
    return true;
}

bool
mainTraverser::TraverseStmt(clang::Stmt* s)
{
    clang::RecursiveASTVisitor<mainTraverser>::TraverseStmt(s);
    return true;
}

bool
mainTraverser::TraverseType(clang::QualType t)
{
    clang::RecursiveASTVisitor<mainTraverser>::TraverseType(t);
    return true;
}

void
mainTraverser::visitFunctionDecl(clang::FunctionDecl fd)
{
    if (checkFunctionIsMRCall(fd))
    {
        instantiated_mrs.emplace_back(fd, std::vector<clang::CallExpr&>);
        this->curr_fd = fd;
    }
    clang::RecursiveASTVisitor<mainTraverser>::VisitFunctionDecl(fd);
    return true;
}

void
mainTraverser::visitCallExpr(clang::CallExpr ce)
{
    if (const clang::FunctionDecl* fd = ce->getDirectCallee)
    {
        if (checkFunctionIsMRCall(fd))
        {
            assert(instantiated_mrs.count(this->curr_fd));
            instantiated_mrs.at(this->curr_fd).push_back(ce);
        }
    }
    clang::RecursiveASTVisitor<mainTraverser>::VisitCallExpr(ce);
    return true;
}

void
gatherReductionOpportunities::HandleTranslationUnit(clang::ASTContext& ctx)
{
    main_traverser.TraverseDecl(ctx.getTranslationUnitDecl());
}

std::unique_ptr<clang::ASTConsumer>
gatherReductionOpportunities::CreateASTConsumer(clang::CompilerInstance& ci,
    llvm::StringRef file)
{
    rw.setSourceMgr(ci.getSourceManager(), ci.getLangOpts());
    return std::make_unique<gatherReductionOpportunities>(rw);
}
