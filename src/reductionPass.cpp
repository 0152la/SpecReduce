#include "reductionPass.hpp"

void
reductionPass::applyReduction(clang::Rewriter& rw)
{
    this->to_modify.dump(rw.getSourceMgr());
    rw.ReplaceText(this->to_modify, new_string);
    for (const clang::FunctionDecl*  fd : this->cleanup_functions)
    {
        functionCleaner fc(fd);
        fc.applyReduction(rw);
    }
}

const clang::FunctionDecl*
reductionPass::addFunctionDREToClean(const clang::DeclRefExpr* dre,
    instantiated_mrs_map_t& instantiated_mrs)
{
    EMIT_DEBUG_INFO("Adding cleanup function " + dre->getNameInfo().getAsString());
    const clang::FunctionDecl* dre_fd =
        llvm::dyn_cast<clang::FunctionDecl>(dre->getDecl());
    assert(dre_fd);
    this->cleanup_functions.push_back(dre_fd);
    if (instantiated_mrs.count(dre_fd))
    {
        instantiated_mrs.at(dre_fd).marked = true;
        for (const clang::DeclRefExpr* recursive_dre :
                instantiated_mrs.at(dre_fd).recursive_calls)
        {
            this->addFunctionDREToClean(recursive_dre, instantiated_mrs);
        }
    }
    return dre_fd;
}

/*******************************************************************************
 * functionCleaner
 ******************************************************************************/

functionCleaner::functionCleaner(const clang::FunctionDecl* fd)
{
    EMIT_DEBUG_INFO("Cleaning orphaned function " + fd->getNameAsString());
    this->to_modify = fd->getSourceRange();
    this->new_string = "";
}

/*******************************************************************************
 * variantReducer
 ******************************************************************************/

variantReducer::variantReducer(variant_decl_t& variant_data,
    variant_instrs_map_t& variant_instrs,
    instantiated_mrs_map_t& instantiated_mrs)
{
    const clang::VarDecl* vd = variant_data.variant_decl;
    clang::SourceRange reduction_range(vd->getBeginLoc(),
        variant_data.instrs.back()->getEndLoc());
    for (const clang::Stmt* var_stmt : variant_data.instrs)
    {
        variant_instruction_t var_instr = variant_instrs.at(var_stmt);
        for (const clang::DeclRefExpr* mr_call_dre : var_instr.mr_calls)
        {
            this->addFunctionDREToClean(mr_call_dre, instantiated_mrs);
        }
        var_instr.marked = true;
    }
    this->to_modify = reduction_range;
    this->new_string = "";
}

/*******************************************************************************
 * recursionReducer
 ******************************************************************************/

recursionReducer::recursionReducer(const clang::DeclRefExpr* dre,
    instantiated_mrs_map_t& instantiated_mrs)
{
    EMIT_DEBUG_INFO("Folding recursion call " + dre->getNameInfo().getAsString());
    this->addFunctionDREToClean(dre, instantiated_mrs);
    this->to_modify = dre->getSourceRange();

    const clang::FunctionDecl* fd = llvm::dyn_cast<clang::FunctionDecl>(dre->getDecl());
    assert(fd);
    assert(instantiated_mrs.count(fd));
    const mrInfo* dre_mri = instantiated_mrs.at(fd).mr_info;
    this->new_string = "metalib::" + dre_mri->family + "::" + dre_mri->name;
}
