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
        globals::variant_instr_index, globals::instantiated_mrs);

    size_t reduction_attempt = 0;
    bool success = false;
    llvm::SmallString<256> tmp_path;
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

        if (!step_reductions.empty())
        {
            reductionStep rs(step_reductions);
            EMIT_DEBUG_INFO("Applying " + std::to_string(rs.opportunities.size()) +
                " selected reductions.");
            for (reductionPass* rp : rs.opportunities)
            {
                rp->applyReduction(rw);
                delete rp;
            }

            int tmp_fd;
            llvm::sys::fs::createTemporaryFile("mtFuzz", ".cpp", tmp_fd, tmp_path);
            llvm::raw_fd_ostream temp_rfo(tmp_fd, true);
            rw.getEditBuffer(rw.getSourceMgr().getMainFileID()).write(temp_rfo);
            temp_rfo.close();
            EMIT_DEBUG_INFO(("Wrote tmp output file " + tmp_path.str()).str());

            interestingExecutor int_exec(tmp_path.str(), globals::interestingness_test_path);
            success = int_exec.runInterestingnessTest();
        }

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
            if (this->offset >= global_reductions.getReductionsSizeByType(this->rd_type))
            {
                if (this->chunk_size == 1)
                {
                    this->rd_type = static_cast<REDUCTION_TYPE>(
                        static_cast<int>(this->rd_type) + 1);
                    this->chunk_size = CHUNK_SIZE_DEF_VALUE;
                    this->offset = 0;
                }
                else
                {
                    this->chunk_size /= CHUNK_SIZE_REDUCE_FACTOR;
                    this->offset = 0;
                }
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
    std::vector<const clang::VarDecl*> r_vds =
        this->rd_type == VARIANT_ELIMINATION
        ? this->selectChunks(global_reductions.variant_decls)
        : std::vector<const clang::VarDecl*>();
    std::vector<size_t> r_viidxs =
        this->rd_type == FAMILY_SHORTENING
        ?  this->selectChunks(global_reductions.variant_instr_index)
        : std::vector<size_t>();
    std::vector<const clang::DeclRefExpr*> r_imrs =
        this->rd_type == RECURSION_REMOVAL
        ? this->selectChunks(global_reductions.recursive_calls)
        : std::vector<const clang::DeclRefExpr*>();
    return reduction_datas_t(r_vds, r_viidxs, r_imrs);
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
