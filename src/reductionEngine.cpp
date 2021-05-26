#include <iostream>

#include "reductionEngine.hpp"
#include "globals.hpp"
#include "compileAndExecute.hpp"

void
reductionEngine::HandleTranslationUnit(clang::ASTContext& ctx)
{
    // Gather declared MRs, but only once
    if (globals::mr_names_list.empty())
    {
        MRGatherer mrg(ctx);
    }

    opportunitiesGatherer og(ctx);
    //assert(false);

    reduction_datas_t global_reductions(globals::variant_decls,
        globals::variant_instr_index, globals::instantiated_mrs,
        globals::fuzzing_regions);

    // Do not reduce variant 0
    global_reductions.variant_decls.erase(
        std::remove_if(std::begin(global_reductions.variant_decls),
            std::end(global_reductions.variant_decls),
            [](const clang::VarDecl* vd) {
                return vd->getNameAsString().find("_0")
                    != std::string::npos; }),
        global_reductions.variant_decls.end());

    // Do not reduce sequence index 0
    global_reductions.variant_instr_index.erase(
        std::remove_if(std::begin(global_reductions.variant_instr_index),
            std::end(global_reductions.variant_instr_index),
            [](size_t idx) { return idx == 0; }),
        global_reductions.variant_instr_index.end());

    size_t reduction_attempt = 0;
    bool success = false;
    llvm::SmallString<256> tmp_path;

    while (!success && this->rd_type <= REDUCTION_TYPE::FUZZING_REDUCTION)
    {
        if (this->chunk_size == CHUNK_SIZE_DEF_VALUE)
        {
            size_t reductions_size =
                global_reductions.getReductionsSizeByType(this->rd_type);
            if (reductions_size <= 1)
            {
                this->chunk_size = reductions_size;
            }
            else
            {
                this->chunk_size = reductions_size / CHUNK_SIZE_INITIAL_FACTOR;
            }
        }

        EMIT_DEBUG_INFO("Reduction loop count " + std::to_string(reduction_attempt) +
            " [TYPE IDX " + std::to_string(this->rd_type) +
            "] [CHUNK SIZE " + std::to_string(this->chunk_size) +
            "] [OFFSET " + std::to_string(this->offset) + "]", 2);

        reduction_datas_t step_reductions = this->selectReductions(global_reductions);

        if (!step_reductions.empty())
        {
            reductionStep rs(step_reductions);
            clang::Rewriter rw_tmp(rw.getSourceMgr(), rw.getLangOpts());
            EMIT_DEBUG_INFO("Applying " + std::to_string(rs.opportunities.size()) +
                " selected reductions.", 2);
            for (reductionPass* rp : rs.opportunities)
            {
                rp->applyReduction(rw_tmp);
                delete rp;
            }

            int tmp_fd;
            std::string tmp_file_name = "mfReduce-" +
                std::to_string(globals::reductions_count) + "-" +
                std::to_string(reduction_attempt) + "-%%%%.cpp";
            ERROR_CHECK(llvm::sys::fs::createUniqueFile(tmp_file_name, tmp_fd, tmp_path));
            llvm::raw_fd_ostream temp_rfo(tmp_fd, true);
            rw_tmp.getEditBuffer(rw_tmp.getSourceMgr().getMainFileID()).write(temp_rfo);
            temp_rfo.close();
            EMIT_DEBUG_INFO(("Wrote tmp output file " + tmp_path).str(), 3);

            interestingExecutor int_exec(tmp_path.str());
            success = int_exec.runInterestingnessTest(globals::expected_return_code);
            EMIT_DEBUG_INFO("Retrieved return code " +
                std::to_string(int_exec.getReturnCode()), 2);

            if (success)
            {
                ERROR_CHECK(llvm::sys::fs::rename(tmp_path, globals::output_file));
                //ERROR_CHECK(llvm::sys::fs::copy_file(tmp_path, globals::output_file));
                EMIT_DEBUG_INFO("Wrote output file " + globals::output_file, 2);
                globals::reduction_success = true;
                globals::reduction_type_progress = this->rd_type;
                this->cleanup();
                return;
            }
            else
            {
                ERROR_CHECK(llvm::sys::fs::remove(tmp_path));
            }
        }
        if (this->offset >= global_reductions.getReductionsSizeByType(this->rd_type)
                || this->chunk_size == 0)
        {
            if (this->chunk_size <= 1)
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

void
reductionEngine::cleanup()
{
    this->cleanMap(globals::variant_decls);
    globals::variant_instr_index.clear();
    this->cleanMap(globals::variant_instrs);
    this->cleanMap(globals::instantiated_mrs);
    this->cleanMap(globals::fuzzing_regions);
}

reductionEngine::reductionEngine(clang::Rewriter& _rw) : rw(_rw)
{
    if (globals::monotonic_reduction)
    {
        this->rd_type = globals::reduction_type_progress;
    }
    else
    {
        this->rd_type = static_cast<REDUCTION_TYPE>(0);
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
    std::vector<std::pair<std::string, const clang::Stmt*>> r_fr =
        this->rd_type == FUZZING_REDUCTION
        ? this->selectChunks(global_reductions.fuzzed_instrs)
        : std::vector<std::pair<std::string, const clang::Stmt*>>();
    return reduction_datas_t(r_vds, r_viidxs, r_imrs, r_fr);
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
