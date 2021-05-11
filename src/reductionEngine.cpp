#include <iostream>

#include "reductionEngine.hpp"
#include "globals.hpp"
#include "compileAndExecute.hpp"

#define CHUNK_SIZE_DEF_VALUE -1
#define CHUNK_SIZE_REDUCE_FACTOR 2
#define CHUNK_SIZE_INITIAL_FACTOR 2

void
reductionEngine::HandleTranslationUnit(clang::ASTContext& ctx)
{
    MRGatherer mrg(ctx);
    opportunitiesGatherer og(ctx);

    reduction_datas_t global_reductions(globals::variant_decls,
        globals::variant_instrs, globals::variant_instr_index,
        globals::instantiated_mrs);

    size_t reduction_attempt = 0;
    bool success = false;
    while (!success && this->rd_type <= REDUCTION_TYPE::RECURSION_REMOVAL &&
            reduction_attempt <= this->max_reduction_attempts)
    {
        if (this->chunk_size == CHUNK_SIZE_DEF_VALUE)
        {
            this->chunk_size =
                global_reductions.getReductionsSizeByType(this->rd_type) / CHUNK_SIZE_INITIAL_FACTOR;
        }

        EMIT_DEBUG_INFO("Reduction loop count " + std::to_string(reduction_attempt) +
            " [TYPE IDX " + std::to_string(this->rd_type) +
            "] [CHUNK SIZE " + std::to_string(this->chunk_size) +
            "] [OFFSET " + std::to_string(this->offset) + "]\r");

        reduction_datas_t step_reductions =
            this->selectReductions(global_reductions);

        reductionStep rs(step_reductions);
        EMIT_DEBUG_INFO("Applying " + std::to_string(rs.opportunities.size()) +
            " selected reductions.");
        for (reductionPass* rp : rs.opportunities)
        {
            rp->applyReduction(rw);
            delete rp;
        }

        llvm::SmallString<256> tmp_path;
        int tmp_fd;
        llvm::sys::fs::createTemporaryFile("mtFuzz", ".cpp", tmp_fd, tmp_path);
        llvm::raw_fd_ostream temp_rfo(tmp_fd, true);
        rw.getEditBuffer(rw.getSourceMgr().getMainFileID()).write(temp_rfo);
        temp_rfo.close();
        EMIT_DEBUG_INFO(("Wrote tmp output file " + tmp_path.str()).str());

        interestingExecutor int_exec(tmp_path.str(), globals::interestingness_test_path);
        success = int_exec.runInterestingnessTest();

        if (success)
        {
            EMIT_DEBUG_INFO("Wrote output file " + globals::output_file);
            globals::reduction_success = true;
            llvm::sys::fs::rename(tmp_path, globals::output_file);
            this->cleanup();
            return;
        }
        else
        {
            llvm::sys::fs::remove(tmp_path);
            if (this->chunk_size == 1)
            {
                this->rd_type = static_cast<REDUCTION_TYPE>(
                    static_cast<int>(this->rd_type) + 1);
                this->chunk_size = CHUNK_SIZE_DEF_VALUE;
                this->offset = 0;
            }
            else if (this->offset >= global_reductions.getReductionsSizeByType(this->rd_type))
            {
                this->chunk_size /= CHUNK_SIZE_REDUCE_FACTOR;
                this->offset = 0;
            }
            else
            {
                this->offset += chunk_size;
            }
            reduction_attempt += 1;
        }
    }
    globals::reduction_success = false;
}

void
reductionEngine::cleanup()
{
    for (std::pair<const clang::VarDecl*, variant_decl_t*> variant_p :
            globals::variant_decls)
    {
        delete variant_p.second;
    }

    for (std::pair<const clang::Stmt*, variant_instruction_t*> instr_p :
            globals::variant_instrs)
    {
        delete instr_p.second;
    }

    for (std::pair<const clang::DeclRefExpr*, instantiated_mr_t*> mr_p :
            globals::instantiated_mrs)
    {
        delete mr_p.second;
    }
}

reduction_datas_t
reductionEngine::selectSequentialChunkReductions(
    const reduction_datas_t& global_reductions)
{
    variant_decls_map_t r_vds = this->rd_type == VARIANT_ELIMINATION ?
        this->selectChunks(global_reductions.variant_decls) : variant_decls_map_t();
    assert(false);
    variant_instr_index_map_t r_viidxs = this->rd_type == FAMILY_SHORTENING ?
        this->selectChunks(global_reductions.variant_instr_index) : variant_instr_index_map_t();
    instantiated_mrs_map_t r_imrs = this->rd_type == RECURSION_REMOVAL ?
        this->selectChunks(global_reductions.instantiated_mrs) : instantiated_mrs_map_t();
    return reduction_datas_t(r_vds, global_reductions.variant_instrs, r_viidxs, r_imrs);
}

void
reductionEngineAction::EndSourceFileAction()
{
    //std::error_code ec;
    //int fd;
    //llvm::raw_fd_ostream of_rfo(globals::output_file, ec);
    //rw.getEditBuffer(rw.getSourceMgr().getMainFileID()).write(of_rfo);
    //of_rfo.close();
    //EMIT_DEBUG_INFO("Wrote output file " + globals::output_file);
}

std::unique_ptr<clang::ASTConsumer>
reductionEngineAction::CreateASTConsumer(clang::CompilerInstance& ci,
    llvm::StringRef file)
{
    rw.setSourceMgr(ci.getSourceManager(), ci.getLangOpts());
    return std::make_unique<reductionEngine>(rw);
}

reduction_datas_t::reduction_datas_t(variant_decls_map_t _vds,
    variant_instr_index_map_t _viidx, instantiated_mrs_map_t _imrs)
{
    this->variant_decls = this->reduceMapKeysToVector(_vds);
    this->variant_instr_index = this->reduceMapKeysToVector(_viidx);
    this->recursive_calls = this->reduceMapKeysToVector(_imrs);

    //for (std::pair<const clang::VarDecl*, variant_decl_t*> vd_pair : _vds)
    //{
        //this->variant_decls.push_back(vd_pair.first);
    //}

    //this->variant_instr_index.resize(_viidx.size());
    //// TODO expand this to 0
    //std::iota(std::begin(this->variant_instr_index),
        //std::end(this->variant_instr_index), 1)

    //for (std::pair<const clang::Stmt*, instantiated_mr_t*> imr_pair : _imrs)
    //{
        //this->instantiated_mrs.push_back(imr_pair.first);
    //}
}

size_t
reduction_datas_t::getReductionsSizeByType(REDUCTION_TYPE rd_type)
{
    switch (rd_type)
    {
        case VARIANT_ELIMINATION:
            return this->variant_decls.size();
        case FAMILY_SHORTENING:
            return this->variant_instr_index.size();
        case RECURSION_REMOVAL:
            return this->recursive_calls.size();
        default:
            assert(false);
    }
}
