#include "opportunitiesGatherer.hpp"

mainTraverser* main_traverser = nullptr;

bool
checkNameIsVariant(std::string name_check)
{
    std::string regex_string = globals::variant_prefix + globals::delimiter + "[0-9]*";
    std::regex variant_name_regex(regex_string, std::regex::grep);
    return (std::regex_match(name_check, variant_name_regex));
}

const clang::ast_type_traits::DynTypedNode
mainTraverser::getBaseParent(const clang::DeclRefExpr* dre)
{
    clang::ASTContext::DynTypedNodeList dre_parents =
        this->ctx.getParents(*dre);
    assert(dre_parents.size() == 1);
    return this->getBaseParent(dre_parents[0]);
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

instantiatedMRVisitor::instantiatedMRVisitor(clang::DeclRefExpr* _dre, const mrInfo* _mri) :
    base_dre(_dre), fd_mri(_mri)
{
    instantiated_mr_t* imr = new instantiated_mr_t(this->base_dre, this->fd_mri);
    //this->base_dre->dump();
    //std::cout << this->fd_mri->getFullName() << std::endl;
    //std::cout << "============================================================" << std::endl;
    globals::instantiated_mrs.emplace(this->base_dre, imr);
    const clang::FunctionDecl* dre_fd =
        llvm::dyn_cast<clang::FunctionDecl>(this->base_dre->getDecl());
    assert(dre_fd);
    clang::RecursiveASTVisitor<instantiatedMRVisitor>::TraverseFunctionDecl(
        const_cast<clang::FunctionDecl*>(dre_fd));
}

bool
instantiatedMRVisitor::VisitDeclRefExpr(clang::DeclRefExpr* dre)
{
    if (clang::FunctionDecl* fd = llvm::dyn_cast<clang::FunctionDecl>(dre->getDecl()))
    {
        if (const mrInfo* fd_mri = checkFunctionIsMRCall(fd))
        {
            globals::instantiated_mrs.at(this->base_dre)->recursive_calls.push_back(dre);
            instantiatedMRVisitor imr_visit(dre, fd_mri);
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

//bool
//mainTraverser::VisitCallExpr(clang::CallExpr* ce)
//{
    //if (clang::FunctionDecl* fd = ce->getDirectCallee())
    //{
        //if (checkFunctionIsMRCall(fd))
        //{
            //instantiatedMRVisitor imr_visit(fd);
        //}
    //}
    //clang::RecursiveASTVisitor<mainTraverser>::VisitCallExpr(ce);
    //return true;
//}

bool
mainTraverser::VisitVarDecl(clang::VarDecl* vd)
{
    std::string var_name = vd->getNameAsString();
    if (checkNameIsVariant(var_name))
    {
        variant_decl_t* vd_t = new variant_decl_t(vd);
        globals::variant_decls.emplace(vd, vd_t);
        this->curr_variant_vd = vd;
        this->curr_vd_index = 0;
    }
    clang::RecursiveASTVisitor<mainTraverser>::VisitVarDecl(vd);
    // TODO consider if curr_variant_vd should be reset
    return true;
}

bool
mainTraverser::VisitDeclRefExpr(clang::DeclRefExpr* dre)
{
    if (clang::VarDecl* vd = llvm::dyn_cast<clang::VarDecl>(dre->getDecl()))
    {
        if (checkNameIsVariant(vd->getNameAsString()))
        {
            const clang::Stmt* var_instr_stmt = this->getBaseParent(dre).get<clang::Stmt>();
            if (!globals::variant_instrs.count(var_instr_stmt))
            {
                variant_instruction_t* var_instr =
                    new variant_instruction_t(var_instr_stmt, vd);
                globals::variant_instrs.emplace(var_instr_stmt, var_instr);
                globals::variant_decls.at(vd)->instrs.push_back(var_instr_stmt);
                if (!globals::variant_instr_index.count(this->curr_vd_index))
                {
                    globals::variant_instr_index.emplace(
                        this->curr_vd_index, std::vector<variant_instruction_t*>());
                }
                globals::variant_instr_index.at(this->curr_vd_index).push_back(var_instr);
                this->curr_vd_index += 1;
            }
        }
    }
    else if (clang::FunctionDecl* fd = llvm::dyn_cast<clang::FunctionDecl>(dre->getDecl()))
    {
        if (const mrInfo* fd_mri = checkFunctionIsMRCall(fd))
        {
            //dre->dump();
            //std::cout << fd_mri->getFullName() << std::endl;
            //std::cout << "============================================================" << std::endl;
            instantiatedMRVisitor imr_visit(dre, fd_mri);

            if (this->curr_variant_vd)
            {
                assert(globals::variant_decls.count(this->curr_variant_vd));
                const clang::Stmt* dre_base_stmt = this->getBaseParent(dre).get<clang::Stmt>();
                if (globals::variant_instrs.count(dre_base_stmt))
                {
                    globals::variant_instrs.at(dre_base_stmt)->mr_calls.push_back(dre);
                    // TODO optionally check that the stmt is in the variant_decl_t object
                }
                else
                {
                    variant_instruction_t* var_instr =
                        new variant_instruction_t(dre_base_stmt, this->curr_variant_vd);
                    var_instr->mr_calls.push_back(dre);
                    globals::variant_instrs.emplace(dre_base_stmt, var_instr);

                    globals::variant_decls.at(this->curr_variant_vd)->instrs.push_back(dre_base_stmt);
                    if (!globals::variant_instr_index.count(this->curr_vd_index))
                    {
                        globals::variant_instr_index.emplace(
                            this->curr_vd_index, std::vector<variant_instruction_t*>());
                    }
                    globals::variant_instr_index.at(this->curr_vd_index).push_back(var_instr);
                    this->curr_vd_index += 1;
                }
            }
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

opportunitiesGatherer::opportunitiesGatherer(clang::ASTContext& ctx)
{

    mr_matcher.addMatcher(
        clang::ast_matchers::functionDecl(
        clang::ast_matchers::isMain())
            .bind("mainFD"), &main_traverser_cb);

    main_traverser = new mainTraverser(ctx);
    mr_matcher.matchAST(ctx);
    delete(main_traverser);
}
