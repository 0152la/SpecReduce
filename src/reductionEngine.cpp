#include "reductionEngine.hpp"
#include "globals.hpp"

#include <iostream>

void
reductionEngine::HandleTranslationUnit(clang::ASTContext& ctx)
{
    MRGatherer mrg(ctx);
    opportunitiesGatherer og(ctx);

    //for (std::pair<const clang::VarDecl*, variant_decl_t> p : globals::variant_decls)
    //{
        //std::cout << "================================================================================" << std::endl;
        //std::cout << p.first->getNameAsString() << std::endl;
        //for (const clang::Stmt* s : p.second.instrs)
        //{
            //s->dump();
        //}
    //}
    //assert(false);

    reductionStep rs(globals::variant_decls, globals::variant_instrs,
        globals::instantiated_mrs);
    for (reductionPass* rp : rs.opportunities)
    {
        rp->applyReduction(rw);
        delete rp;
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
