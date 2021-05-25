#include "globals.hpp"

reduction_datas_t::reduction_datas_t(const variant_decls_map_t _vds,
    const variant_instr_index_map_t _viidx,
    const instantiated_mrs_map_t _imrs,
    const fuzzing_regions_map_t _fr)
{
    this->variant_decls = this->reduceMapKeysToVector(_vds);
    this->variant_instr_index = this->reduceMapKeysToVector(_viidx);
    this->recursive_calls = this->reduceMapKeysToVector(_imrs);
    this->fuzzed_instrs = this->reduceFuzzingRegionsToInstr(_fr);
}

size_t
reduction_datas_t::getReductionsSizeByType(REDUCTION_TYPE rd_type) const
{
    switch (rd_type)
    {
        case VARIANT_ELIMINATION:
            return this->variant_decls.size();
        case FAMILY_SHORTENING:
            return this->variant_instr_index.size();
        case RECURSION_REMOVAL:
            return this->recursive_calls.size();
        case FUZZING_REDUCTION:
            return this->fuzzed_instrs.size();
        default:
            assert(false);
    }
}

bool
reduction_datas_t::empty() const
{
    return this->variant_decls.empty() && this->variant_instr_index.empty()
        && this->recursive_calls.empty() && this->fuzzed_instrs.empty();
}

std::vector<std::pair<std::string, const clang::Stmt*>>
reduction_datas_t::reduceFuzzingRegionsToInstr(
    const std::map<std::string, fuzzing_region_t*> fuzzing_data)
{
    std::vector<std::pair<std::string, const clang::Stmt*>> fuzzed_instrs;
    for (std::pair<std::string, fuzzing_region_t*> fuzzing_data_pair
            : fuzzing_data)
    {
        for (const clang::Stmt* instr : fuzzing_data_pair.second->instrs)
        {
            fuzzed_instrs.emplace_back(fuzzing_data_pair.first, instr);
        }
    }
    return fuzzed_instrs;
}

reduce_fn_data::reduce_fn_data(const clang::FunctionDecl* _rd_fn) :
    reduce_fn(_rd_fn)
{
    for (const clang::ParmVarDecl* param : _rd_fn->parameters())
    {
        std::string param_type_name = param->getOriginalType().getAsString();
        this->reduce_fn_arg_types.push_back(param_type_name);
        globals::reduce_fn_param_types.insert(param_type_name);
    }
}

