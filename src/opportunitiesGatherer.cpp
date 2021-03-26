#include "opportunitiesGatherer.hpp"
#include "globals.hpp"

mainTraverser* main_traverser = nullptr;

bool
checkFunctionIsMRCall(const clang::FunctionDecl* fd)
{
    std::string fd_name = fd->getNameAsString();
    std::string::reverse_iterator r_it = fd_name.rbegin();
    size_t delim_count = 0, expected_count = 2, char_count = 0;
    while (delim_count < expected_count && r_it != fd_name.rend())
    {
        if (*r_it == globals::delimiter)
        {
            delim_count += 1;
        }
        r_it = std::next(r_it);
        char_count += 1;
    }
    fd_name = fd_name.substr(0, fd_name.size() - char_count);
    return globals::mr_names_list.count(fd_name);
}

bool
checkNameIsVariant(std::string name_check)
{
    std::string regex_string = globals::variant_prefix + globals::delimiter + "[0-9]*";
    std::regex variant_name_regex(regex_string, std::regex::grep);
    return (std::regex_match(name_check, variant_name_regex));
}

const clang::ast_type_traits::DynTypedNode
mainTraverser::getBaseParent(const clang::ast_type_traits::DynTypedNode dyn_node)
{
    clang::ASTContext::DynTypedNodeList node_parents =
        this->ctx.getParents(dyn_node);
    assert(node_parents.size() == 1);
    if (node_parents[0].get<clang::CompoundStmt>() == this->main_child)
    {
        return dyn_node;
    }
    return this->getBaseParent(node_parents[0]);
}

instantiatedMRVisitor::instantiatedMRVisitor(clang::FunctionDecl* _fd) :
    base_fd(_fd)
{
    globals::instantiated_mrs.emplace(this->base_fd, std::vector<const clang::DeclRefExpr*>());
    clang::RecursiveASTVisitor<instantiatedMRVisitor>::TraverseFunctionDecl(this->base_fd);
}

bool
instantiatedMRVisitor::VisitDeclRefExpr(clang::DeclRefExpr* dre)
{
    if (clang::FunctionDecl* fd = llvm::dyn_cast<clang::FunctionDecl>(dre->getDecl()))
    {
        if (checkFunctionIsMRCall(fd))
        {
            globals::instantiated_mrs.at(this->base_fd).push_back(dre);
            instantiatedMRVisitor imr_visit(fd);
        }
    }
    clang::RecursiveASTVisitor<instantiatedMRVisitor>::VisitDeclRefExpr(dre);
    return true;
}

bool
mainTraverser::VisitCompoundStmt(clang::CompoundStmt* cs)
{
    if (!main_traverser->getMainChild())
    {
        clang::ASTContext::DynTypedNodeList node_parents =
            this->ctx.getParents(*cs);
        if (node_parents.size() == 1 &&
                node_parents[0].get<clang::FunctionDecl>()->isMain())
        {
            main_traverser->setMainChild(cs);
        }
    }
    clang::RecursiveASTVisitor<mainTraverser>::VisitCompoundStmt(cs);
    return true;
}

bool
mainTraverser::VisitCallExpr(clang::CallExpr* ce)
{
    if (clang::FunctionDecl* fd = ce->getDirectCallee())
    {
        if (checkFunctionIsMRCall(fd))
        {
            instantiatedMRVisitor imr_visit(fd);
        }
    }
    clang::RecursiveASTVisitor<mainTraverser>::VisitCallExpr(ce);
    return true;
}

bool
mainTraverser::VisitVarDecl(clang::VarDecl* vd)
{
    std::string var_name = vd->getNameAsString();
    if (checkNameIsVariant(var_name))
    {
        globals::variant_instrs.emplace(var_name, std::vector<const clang::SourceLocation&>());
        this->curr_variant_name = var_name;
    }
    clang::RecursiveASTVisitor<mainTraverser>::VisitVarDecl(vd);
    return true;
}

bool
mainTraverser::VisitDeclRefExpr(clang::DeclRefExpr* dre)
{
    if (const clang::VarDecl* vd = llvm::dyn_cast<clang::VarDecl>(dre->getDecl()))
    {
        std::string var_name = vd->getNameAsString();
        if (checkNameIsVariant(var_name))
        {
            clang::ASTContext::DynTypedNodeList vd_parents =
                this->ctx.getParents(*vd);
            assert(vd_parents.size() == 1);
            globals::variant_instrs.at(var_name).emplace_back(
                this->getBaseParent(vd_parents[0]).get<clang::Stmt>().getSourceRange());

        }
    }
    clang::RecursiveASTVisitor<mainTraverser>::VisitDeclRefExpr(dre);
    return true;
}

void
mainTraverserCallback::run(const clang::ast_matchers::MatchFinder::MatchResult& Result)
{
    clang::FunctionDecl* fd =
        const_cast<clang::FunctionDecl*>(
            Result.Nodes.getNodeAs<clang::FunctionDecl>("mainFD"));
    assert(fd);
    assert(!main_traverser->hasVisited());
    main_traverser->TraverseDecl(fd);
    main_traverser->setVisited();
}

void
MRLogger::run(const clang::ast_matchers::MatchFinder::MatchResult& Result)
{
    const clang::FunctionDecl* fd =
        Result.Nodes.getNodeAs<clang::FunctionDecl>("metaRel");
    globals::mr_names_list.emplace(fd->getQualifiedNameAsString());
}

opportunitiesGatherer::opportunitiesGatherer(clang::ASTContext& ctx)
{
    mr_matcher.addMatcher(
        clang::ast_matchers::functionDecl(
        clang::ast_matchers::hasAncestor(
        clang::ast_matchers::namespaceDecl(
        clang::ast_matchers::hasName(
        "metalib"))))
            .bind("metaRel"), &mr_logger);

    mr_matcher.addMatcher(
        clang::ast_matchers::functionDecl(
        clang::ast_matchers::isMain())
            .bind("mainFD"), &main_traverser_cb);

    main_traverser = new mainTraverser(ctx);
    mr_matcher.matchAST(ctx);
    delete(main_traverser);
}
