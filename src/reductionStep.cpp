#include "reductionStep.hpp"

reductionStep::reductionStep(reduction_datas_t _rds)
{
    std::vector<reduction_data_t*> processed_reductions;
    for (const clang::VarDecl* rd_var : _rds.variant_decls)
    {
        variant_decl_t* variant_data = globals::variant_decls.at(rd_var);
        std::string var_name = variant_data->variant_decl->getNameAsString();
        if (var_name.find("_0") != std::string::npos)
        {
            EMIT_DEBUG_INFO("Skipping variant id 0.", 3);
            continue;
        }
        assert(!variant_data->marked);
        EMIT_DEBUG_INFO("Adding variant eliminator for variant name " + var_name, 3);
        this->opportunities.push_back(new variantReducer(variant_data, processed_reductions));
    }

    for (size_t rd_idx : _rds.variant_instr_index)
    {
        if (rd_idx == 0)
        {
            EMIT_DEBUG_INFO("Skipping sequence index 0.", 3);
            continue;
        }
        EMIT_DEBUG_INFO("Reducing function sequence at index " +
            std::to_string(rd_idx), 3);
        for (variant_instruction_t* var_instr : globals::variant_instr_index.at(rd_idx))
        {
            if (var_instr->marked)
            {
                EMIT_DEBUG_INFO("Skipping marked instruction index " +
                    std::to_string(rd_idx) + " for variant " +
                    var_instr->assoc_variant->getNameAsString()+ ".", 3);
            }
            else
            {
                EMIT_DEBUG_INFO("Reducing instruction index " +
                    std::to_string(rd_idx) + " for variant " +
                    var_instr->assoc_variant->getNameAsString()+ ".", 3);
                this->opportunities.push_back(
                    new instructionReducer(var_instr, processed_reductions));
            }
        }
    }

    for (const clang::DeclRefExpr* rd_rec_call : _rds.recursive_calls)
    {
        instantiated_mr_t* mr_instance_data =
            globals::instantiated_mrs.at(rd_rec_call);
        if (mr_instance_data->marked)
        {
            EMIT_DEBUG_INFO("Skipping recursion in marked function " +
                mr_instance_data->mr_decl->getNameAsString(), 3);
            continue;
        }
        EMIT_DEBUG_INFO("Adding recursion folder for call " +
            rd_rec_call->getDecl()->getNameAsString() +
            " in function " + mr_instance_data->mr_decl->getNameAsString(), 3);
        this->opportunities.push_back(
            new recursionReducer(
                mr_instance_data->calling_dre, processed_reductions));
    }

    for (reduction_data_t* rd : processed_reductions)
    {
        rd->marked = false;
    }
}
