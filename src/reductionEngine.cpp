#include "reductionEngine.hpp"
#include "globals.hpp"

void
reductionEngine::HandleTranslationUnit(clang::ASTContext& ctx)
{
    opportunitiesGatherer og(ctx);

    for (std::string s : globals::mr_names_list)
    {
        std::cout << s << std::endl;
    }

    for (std::pair<std::string, std::vector<clang::SourceRange&>> p : globals::variant_instrs)
    //for (std::pair<std::string, std::vector<const clang::Stmt*>> p : globals::variant_instrs)
    {
        if (p.first.find("_0") == std::string::npos)
        {
            continue;
        }
        globals::opportunities.push_back(new variantReducer(p.first, this->rw));
        //std::cout << p.first << std::endl;
        //std::cout << "--------------------" << std::endl;
        //for (const clang::Stmt* s : p.second)
        //{
            ////s->dump();
        //}
        //std::cout << "====================" << std::endl;
    }

    //for (std::pair<const clang::FunctionDecl*, std::vector<const clang::DeclRefExpr*>> p :
            //globals::instantiated_mrs)
    //{
        //std::cout << p.first->getNameAsString() << std::endl;
        //std::cout << "--------------------" << std::endl;
        //for (const clang::DeclRefExpr* dre : p.second)
        //{
            //std::cout <<dre->getDecl()->getNameAsString() << std::endl;
        //}
        //std::cout << "====================" << std::endl;
    //}
    //
    for (reductionStep* rs : globals::opportunities)
    {
        rs->applyReduction(this->rw);
        delete(rs);
    }
}

std::unique_ptr<clang::ASTConsumer>
reductionEngineAction::CreateASTConsumer(clang::CompilerInstance& ci,
    llvm::StringRef file)
{
    clang::Rewriter rw(ci.getSourceManager(), ci.getLangOpts());
    return std::make_unique<reductionEngine>(rw);
}
