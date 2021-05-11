#include "reductionStep.hpp"

reductionStep::reductionStep(reduction_datas_t _rds) :
        variant_decls(_rds.variant_decls),
        variant_instr_index(_rds.variant_instr_index),
        variant_instrs(_rds.variant_instrs),
        instantiated_mrs(_rds.instantiated_mrs)
{
    for (std::pair<const clang::VarDecl*, variant_decl_t*> variant_data_pair :
            this->variant_decls)
    {
        variant_decl_t* variant_data = variant_data_pair.second;
        std::string var_name = variant_data->variant_decl->getNameAsString();
        if (var_name.find("_0") != std::string::npos)
        {
            EMIT_DEBUG_INFO("Skipping variant id 0.");
            continue;
        }
        assert(!variant_data->marked);
        EMIT_DEBUG_INFO("Adding variant eliminator for variant name " + var_name);
        this->opportunities.push_back(
            new variantReducer(variant_data, this->variant_instrs, this->instantiated_mrs));
    }

    for (std::pair<size_t, std::vector<variant_instruction_t*>> variant_instr_index_pair :
            this->variant_instr_index)
    {
        if (variant_instr_index_pair.first == 0)
        {
            EMIT_DEBUG_INFO("Skipping sequence index 0.");
            continue;
        }
        EMIT_DEBUG_INFO("Reducing function sequence at index " +
            std::to_string(variant_instr_index_pair.first));
        for (variant_instruction_t* var_instr : variant_instr_index_pair.second)
        {
            if (var_instr->marked)
            {
                EMIT_DEBUG_INFO("Skipping marked instruction index " +
                    std::to_string(variant_instr_index_pair.first) +
                    " for variant " +
                    var_instr->assoc_variant->getNameAsString()+ ".");
            }
            else
            {
                EMIT_DEBUG_INFO("Reducing instruction index " +
                    std::to_string(variant_instr_index_pair.first) +
                    " for variant " +
                    var_instr->assoc_variant->getNameAsString()+ ".");
                this->opportunities.push_back(
                    new instructionReducer(var_instr, this->instantiated_mrs));
            }
        }
    }

    for (std::pair<const clang::DeclRefExpr*, instantiated_mr_t*>
            mr_instance_pair : this->instantiated_mrs)
    {
        instantiated_mr_t* mr_instance_data = mr_instance_pair.second;
        if (mr_instance_data->marked)
        {
            EMIT_DEBUG_INFO("Skipping recursion in marked function " +
                mr_instance_data->mr_decl->getNameAsString());
            continue;
        }
        EMIT_DEBUG_INFO("Adding recursion folder for call " +
            mr_instance_pair.first->getDecl()->getNameAsString() +
            " in function " + mr_instance_data->mr_decl->getNameAsString());
        this->opportunities.push_back(
            new recursionReducer(
                mr_instance_data->calling_dre, this->instantiated_mrs));
    }
}
