#include "opportunitiesGatherer.hpp"

mainTraverser* main_traverser = nullptr;

const std::string fuzz_start_marker_name = "mfr_fuzz_start";
const std::string fuzz_end_marker_name = "mfr_fuzz_end";

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
    const clang::FunctionDecl* dre_fd =
        llvm::dyn_cast<clang::FunctionDecl>(this->base_dre->getDecl());
    assert(dre_fd);
    if (!checkIsTransplanted(dre_fd))
    {
        return;
    }
    instantiated_mr_t* imr = new instantiated_mr_t(this->base_dre, this->fd_mri);
    globals::instantiated_mrs.emplace(this->base_dre, imr);
    clang::RecursiveASTVisitor<instantiatedMRVisitor>::TraverseFunctionDecl(
        const_cast<clang::FunctionDecl*>(dre_fd));
}

bool
instantiatedMRVisitor::VisitDeclRefExpr(clang::DeclRefExpr* dre)
{
    if (clang::FunctionDecl* fd = llvm::dyn_cast<clang::FunctionDecl>(dre->getDecl());
        fd && checkIsTransplanted(fd))
    {
        if (const mrInfo* fd_mri = checkFunctionIsMRCall(fd);
            fd_mri && fd_mri->type.compare("checks"))
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
    { clang::ASTContext::DynTypedNodeList node_parents =
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
        bool is_checked_non_mr = globals::checked_non_mrs.count(fd);
        const mrInfo* fd_mri = is_checked_non_mr
            ? nullptr : checkFunctionIsMRCall(fd);
        if (fd_mri && fd_mri->type.compare("checks"))
        {
            instantiatedMRVisitor imr_visit(dre, fd_mri);

            if (this->curr_variant_vd)
            {
                assert(globals::variant_decls.count(this->curr_variant_vd));
                const clang::Stmt* dre_base_stmt = this->getBaseParent(dre).get<clang::Stmt>();
                if (globals::variant_instrs.count(dre_base_stmt)) // && !fd_mri->is_base)
                {
                    if (!fd_mri->is_base)
                    {
                        globals::variant_instrs.at(dre_base_stmt)->mr_calls.push_back(dre);
                    }
                    // TODO optionally check that the stmt is in the variant_decl_t object
                }
                else
                {
                    variant_instruction_t* var_instr =
                        new variant_instruction_t(dre_base_stmt, this->curr_variant_vd);
                    if (!fd_mri->is_base)
                    {
                        var_instr->mr_calls.push_back(dre);
                    }
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
        else if (!fd_mri && !is_checked_non_mr)
        {
            globals::checked_non_mrs.insert(fd);
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

/*******************************************************************************
 * Fuzzing reduction helper functions
 ******************************************************************************/

reductionFuncParamFinder::reductionFuncParamFinder(
    const clang::FunctionDecl* root_fd)
{
    this->remaining_param_types = globals::reduce_fn_param_types;
    this->TraverseFunctionDecl(const_cast<clang::FunctionDecl*>(root_fd));
    assert(this->remaining_param_types.empty());
}

bool
reductionFuncParamFinder::VisitVarDecl(clang::VarDecl* vd)
{
    for (auto it = this->remaining_param_types.begin();
        it != this->remaining_param_types.end(); ++it)
    {
        if (!vd->getType().getAsString().compare(*it))
        {
            this->concrete_param_vars.emplace(*it, vd->getNameAsString());
            this->remaining_param_types.erase(it);
            break;
        }
    }
    return !this->remaining_param_types.empty();
}

prevFuzzedChecker::prevFuzzedChecker(const clang::Stmt* s)
{
    this->TraverseStmt(const_cast<clang::Stmt*>(s));
}

bool
prevFuzzedChecker::VisitCallExpr(clang::CallExpr* ce)
{
    if (const clang::FunctionDecl* fd = llvm::dyn_cast<clang::FunctionDecl>(ce->getDirectCallee()))
    {
        if (globals::reduce_fn_names.count(fd->getQualifiedNameAsString()))
        {
            this->was_fuzzed = true;
        }
    }
    return false;
}

std::vector<const clang::Stmt*>
fuzzerGathererCallback::getFuzzingStmts(const clang::CallExpr* start_ce,
    const clang::CompoundStmt* cs)
{
    bool in_sequence = false;
    std::vector<const clang::Stmt*> fuzzing_instrs;

    for (clang::CompoundStmt::const_body_iterator it = cs->body_begin();
        it != cs->body_end(); ++it)
    {
        const clang::Stmt* child_expr = *it;
        if (const clang::ExprWithCleanups* ewc =
            llvm::dyn_cast<clang::ExprWithCleanups>(child_expr))
        {
            child_expr = ewc->getSubExpr();
        }

        const clang::CallExpr* ce = llvm::dyn_cast<clang::CallExpr>(child_expr);
        child_expr->dump();
        if (ce == start_ce)
        {
            in_sequence = true;
        }
        else if (in_sequence && ce)
        {
            if (ce->getDirectCallee()->getQualifiedNameAsString()
                    .find(fuzz_end_marker_name) != std::string::npos)
            {
                return fuzzing_instrs;
            }
        }
        else if (in_sequence && !llvm::dyn_cast<clang::NullStmt>(child_expr))
        {
            if (llvm::dyn_cast<clang::NullStmt>(child_expr))
            {
                continue;
            }
            prevFuzzedChecker pfc(child_expr);
            if (pfc.was_fuzzed)
            {
                continue;
            }
            fuzzing_instrs.push_back(child_expr);
        }
    }
    assert(false);
}

void
literalFinder::run(const clang::ast_matchers::MatchFinder::MatchResult& Result)
{
    assert(this->parsed_string.empty());
    const clang::StringLiteral* sl = Result.Nodes.getNodeAs<clang::StringLiteral>("stringLiteral");
    this->parsed_string = sl->getString();
}

void
fuzzerGathererCallback::run(const clang::ast_matchers::MatchFinder::MatchResult& Result)
{
    const clang::FunctionDecl* rootFD =
        Result.Nodes.getNodeAs<clang::FunctionDecl>("rootFD");
    assert(rootFD);
    const clang::CompoundStmt* rootCS =
        Result.Nodes.getNodeAs<clang::CompoundStmt>("rootCS");
    assert(rootCS);
    const clang::CallExpr* start_ce =
        Result.Nodes.getNodeAs<clang::CallExpr>("fuzzStart");

    clang::ast_matchers::MatchFinder lit_finder;
    literalFinder literal_finder_cb;

    lit_finder.addMatcher(
        clang::ast_matchers::expr(
        clang::ast_matchers::hasDescendant(
        clang::ast_matchers::stringLiteral()
            .bind("stringLiteral"))), &literal_finder_cb);

    for (const clang::Expr* e : start_ce->arguments())
    {
        lit_finder.match(*e, main_traverser->ctx);
    }
    assert(!literal_finder_cb.parsed_string.empty());

    reductionFuncParamFinder rfpm(rootFD);

    fuzzing_region_t* new_fuzz_region =
        new fuzzing_region_t(literal_finder_cb.parsed_string,
            getFuzzingStmts(start_ce, rootCS),
            rfpm.concrete_param_vars, main_traverser->ctx);
    globals::fuzzing_regions.emplace(literal_finder_cb.parsed_string, new_fuzz_region);
}

void
fuzzReductionFunctionGatherer::run(const clang::ast_matchers::MatchFinder::MatchResult& Result)
{
    const clang::FunctionDecl* reduce_func =
        Result.Nodes.getNodeAs<clang::FunctionDecl>("reduceFunc");
    assert(reduce_func);
    if (!reduce_func->getNameAsString().compare(fuzz_start_marker_name) ||
            !reduce_func->getNameAsString().compare(fuzz_end_marker_name))
    {
        return;
    }
    reduce_fn_data* rfd = new reduce_fn_data(reduce_func);
    globals::reduce_fn_list.emplace(
        reduce_func->getReturnType().getAsString(), rfd);
    globals::reduce_fn_names.insert(reduce_func->getQualifiedNameAsString());
}

opportunitiesGatherer::opportunitiesGatherer(clang::ASTContext& ctx)
{
    // Gather reduction function data, but only once
    if (globals::reduce_fn_list.empty())
    {
        mr_matcher.addMatcher(
            clang::ast_matchers::functionDecl(
            clang::ast_matchers::matchesName("mfr_*"))
                .bind("reduceFunc"), &fuzz_reduction_fns_cb);
    }

    mr_matcher.addMatcher(
        clang::ast_matchers::callExpr(
        clang::ast_matchers::allOf(
            clang::ast_matchers::callee(
            clang::ast_matchers::functionDecl(
            clang::ast_matchers::hasName(fuzz_start_marker_name))),

            clang::ast_matchers::hasAncestor(
            clang::ast_matchers::compoundStmt(
            clang::ast_matchers::hasParent(
            clang::ast_matchers::functionDecl()
                .bind("rootFD")))
                .bind("rootCS"))))
                .bind("fuzzStart"), &fuzzer_gatherer_cb);

    mr_matcher.addMatcher(
        clang::ast_matchers::functionDecl(
        clang::ast_matchers::isMain())
            .bind("mainFD"), &main_traverser_cb);

    main_traverser = new mainTraverser(ctx);
    mr_matcher.matchAST(ctx);
    delete(main_traverser);
}
