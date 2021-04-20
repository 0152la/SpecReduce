#include "reductionStep.hpp"

reductionStep::reductionStep(variant_decls_map_t _vd_map, variant_instrs_map_t _vi_map,
    instantiated_mrs_map_t _imrs_map) :
        variant_decls(_vd_map), variant_instrs(_vi_map), instantiated_mrs(_imrs_map)
{
    for (std::pair<const clang::VarDecl*, variant_decl_t> variant_data_pair :
            this->selectReductions(this->variant_decls))
    {
        variant_decl_t variant_data = variant_data_pair.second;
        std::string var_name = variant_data.variant_decl->getNameAsString();
        if (var_name.find("_0") != std::string::npos)
        {
            EMIT_DEBUG_INFO("Skipping variant id 0.");
            continue;
        }
        assert(!variant_data.marked);
        EMIT_DEBUG_INFO("Adding variant eliminator for variant name " + var_name);
        this->opportunities.push_back(
            new variantReducer(variant_data, this->variant_instrs, this->instantiated_mrs));

        for (const clang::Stmt* var_stmt : variant_data.instrs)
        {
            assert(this->variant_instrs.count(var_stmt));
            variant_instruction_t var_instr = this->variant_instrs.at(var_stmt);
            var_instr.marked = true;
            //for (const clang::DeclRefExpr* instr_dre : var_instr.mr_calls)
            //{
                //const clang::FunctionDecl* dre_fd =
                    //llvm::dyn_cast<clang::FunctionDecl>(instr_dre->getDecl());
                //assert(dre_fd);
                //assert(this->instantiated_mrs.count(dre_fd));
                //this->instantiated_mrs.at(dre_fd).marked = true;
            //}
        }
    }

    instantiated_mrs_map_t recursive_instantiated_mrs;
    std::for_each(this->instantiated_mrs.begin(), this->instantiated_mrs.end(),
        [&recursive_instantiated_mrs]
            (std::pair<const clang::FunctionDecl*, instantiated_mr_t> p)
        {
            if (!p.second.recursive_calls.empty())
            {
                recursive_instantiated_mrs.insert(p);
            }
        });

    //for (std::pair<const clang::FunctionDecl*, instantiated_mr_t>
            //mr_instance_pair : recursive_instantiated_mrs)
    //{
        //std::cout << "========================================" << std::endl;
        //std::cout << mr_instance_pair.first->getNameAsString() << std::endl;
        //std::cout << mr_instance_pair.second.marked << std::endl;
    //}
    //assert(false);

    for (std::pair<const clang::FunctionDecl*, instantiated_mr_t>
            mr_instance_pair : this->selectReductions(recursive_instantiated_mrs))
    {
        instantiated_mr_t mr_instance_data = mr_instance_pair.second;
        if (mr_instance_data.marked)
        {
            EMIT_DEBUG_INFO("Skipping recursion in marked function " +
                mr_instance_data.mr_decl->getNameAsString());
            continue;
        }
        EMIT_DEBUG_INFO("Performing recursion reduction function " +
            mr_instance_data.mr_decl->getNameAsString());
        for (const clang::DeclRefExpr* dre : mr_instance_data.recursive_calls)
        {
            EMIT_DEBUG_INFO("Adding recursion folder for call " +
                dre->getDecl()->getNameAsString() + " in function " +
                mr_instance_data.mr_decl->getNameAsString());
            this->opportunities.push_back(new recursionReducer(dre, this->instantiated_mrs));
        }
    }
}
