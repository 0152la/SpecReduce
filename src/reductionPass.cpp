#include "reductionPass.hpp"

/**
 * Perform the reduction operation, replacing the set range with the set content
 *
 * \param rw { A Clang Rewriter corresponding to the test case to reduce }
 */

void
reductionPass::applyReduction(clang::Rewriter& rw)
{
    //this->to_modify.dump(rw.getSourceMgr());
    rw.ReplaceText(this->to_modify, new_string);
    for (const clang::FunctionDecl*  fd : this->cleanup_functions)
    {
        functionCleaner fc(fd);
        fc.applyReduction(rw);
    }
}

/**
 * Helper function to clear any functions that are now dead code due to another
 * reduction pass. Additionally marks those functions so that further reduction
 * steps do not attempt to reduce them
 *
 * \param dre { A Clang DeclRefExpr representing a call to the function to reduce }
 * \param instantiated_mrs { A list of all gathered instantiated MRs }
 */

void
reductionPass::addFunctionDREToClean(const clang::DeclRefExpr* dre,
    instantiated_mrs_map_t& instantiated_mrs)
{
    EMIT_DEBUG_INFO("Adding cleanup function " + dre->getNameInfo().getAsString());
    assert(instantiated_mrs.count(dre));
    instantiated_mr_t* imr = instantiated_mrs.at(dre);
    this->cleanup_functions.push_back(imr->mr_decl);
    imr->marked = true;
    for (const clang::DeclRefExpr* recursive_dre : imr->recursive_calls)
    {
        this->addFunctionDREToClean(recursive_dre, instantiated_mrs);
    }
}

/*******************************************************************************
 * functionCleaner
 ******************************************************************************/

/**
 * Completely removes a function declaration; used primarily to clean-up
 * orphaned functions after a prior reduction passs
 */

functionCleaner::functionCleaner(const clang::FunctionDecl* fd)
{
    EMIT_DEBUG_INFO("Cleaning orphaned function " + fd->getNameAsString());
    this->to_modify = fd->getSourceRange();
    this->new_string = "";
}

/*******************************************************************************
 * variantReducer
 ******************************************************************************/

/**
 * Reduces one whole variant; additionally marks contained instantiated MRs for
 * deletion
 */

variantReducer::variantReducer(variant_decl_t* variant_data,
    variant_instrs_map_t& variant_instrs,
    instantiated_mrs_map_t& instantiated_mrs)
{
    const clang::VarDecl* vd = variant_data->variant_decl;
    clang::SourceRange reduction_range(vd->getBeginLoc(),
        variant_data->instrs.back()->getEndLoc());
    for (const clang::Stmt* var_stmt : variant_data->instrs)
    {
        variant_instruction_t* var_instr = variant_instrs.at(var_stmt);
        for (const clang::DeclRefExpr* mr_call_dre : var_instr->mr_calls)
        {
            this->addFunctionDREToClean(mr_call_dre, instantiated_mrs);
        }
        var_instr->marked = true;
    }
    this->to_modify = reduction_range;
    this->new_string = "";
}

/*******************************************************************************
 * instructionReducer
 ******************************************************************************/

instructionReducer::instructionReducer(variant_instruction_t* instr,
    instantiated_mrs_map_t& instantiated_mrs)
{
    for (const clang::DeclRefExpr* mr_dre : instr->mr_calls)
    {
        this->addFunctionDREToClean(mr_dre, instantiated_mrs);
    }
    this->to_modify = instr->instr->getSourceRange();
    this->new_string = "";
}

/*******************************************************************************
 * recursionReducer
 ******************************************************************************/

/**
 * Folds a recursive MR call into a base call, additionally removing function
 * calls orphaned by removing the original function call
 */

recursionReducer::recursionReducer(const clang::DeclRefExpr* dre,
    instantiated_mrs_map_t& instantiated_mrs)
{
    assert(instantiated_mrs.count(dre));
    EMIT_DEBUG_INFO("Folding recursion call " + dre->getNameInfo().getAsString());
    this->addFunctionDREToClean(dre, instantiated_mrs);
    this->to_modify = dre->getSourceRange();

    const mrInfo* base_mri = instantiated_mrs.at(dre)->mr_info->getBaseMR(globals::mr_names_list);
    this->new_string = "metalib::" + base_mri->getFullName();
}
