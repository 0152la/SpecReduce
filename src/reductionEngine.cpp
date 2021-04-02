#include "reductionEngine.hpp"
#include "globals.hpp"

void
reductionEngine::HandleTranslationUnit(clang::ASTContext& ctx)
{
    opportunitiesGatherer og(ctx);

    for (variant_instrs_elem_t p : globals::variant_instrs)
    {
        std::string var_name = p.first->getNameAsString();
        if (var_name.find("_0") != std::string::npos)
        {
            continue;
        }
        EMIT_DEBUG_INFO("Adding variant eliminator for variant name " + var_name);
        globals::opportunities.push_back(new variantReducer(p.first));
    }

    for (std::pair<const clang::FunctionDecl*,
            std::vector<const clang::DeclRefExpr*>> p :
                globals::instantiated_mrs)
    {
        for (const clang::DeclRefExpr* dre : p.second)
        {
            EMIT_DEBUG_INFO("Adding recursion folder for call " +
                dre->getDecl()->getNameAsString() + " in function " +
                p.first->getNameAsString());
            globals::opportunities.push_back(new recursionReducer(dre));
        }
    }

    for (reductionStep* rs : globals::opportunities)
    {
        rs->applyReduction(this->rw);
        delete(rs);
    }
}

void
reductionEngineAction::EndSourceFileAction()
{
    std::error_code ec;
    int fd;
    llvm::raw_fd_ostream of_rfo(globals::output_file, ec);
    rw.getEditBuffer(rw.getSourceMgr().getMainFileID()).write(of_rfo);
    of_rfo.close();
    EMIT_DEBUG_INFO("Wrote output file " + globals::output_file);
}

std::unique_ptr<clang::ASTConsumer>
reductionEngineAction::CreateASTConsumer(clang::CompilerInstance& ci,
    llvm::StringRef file)
{
    rw.setSourceMgr(ci.getSourceManager(), ci.getLangOpts());
    return std::make_unique<reductionEngine>(rw);
}
