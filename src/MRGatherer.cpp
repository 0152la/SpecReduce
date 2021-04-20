#include "MRGatherer.hpp"

mrInfo::mrInfo(std::string _qual_name, bool _base) : is_base(_base)
{
    std::string delim = "::";
    std::string tokenized_name[4];
    const size_t check_family_idx = 1, check_name_idx = 2;
    const size_t mr_family_idx = 2, mr_name_idx = 3;
    size_t token_idx = 0;
    size_t pos = _qual_name.find(delim);
    while (pos != std::string::npos)
    {
        tokenized_name[token_idx] = _qual_name.substr(0, pos);
        token_idx += 1;
        _qual_name = _qual_name.substr(pos + delim.size());
        pos = _qual_name.find(delim);
    }
    assert(!_qual_name.empty());
    tokenized_name[token_idx] = _qual_name;
    if (!tokenized_name[check_family_idx].compare("checks"))
    {
        assert(token_idx == check_family_idx + 1);
        this->family = tokenized_name[check_family_idx];
        this->name = tokenized_name[check_name_idx];
    }
    else
    {
        assert(token_idx == mr_family_idx + 1);
        this->family = tokenized_name[mr_family_idx];
        this->name = tokenized_name[mr_name_idx];
    }
}

void
MRLogger::run(const clang::ast_matchers::MatchFinder::MatchResult& Result)
{
    const clang::FunctionDecl* fd =
        Result.Nodes.getNodeAs<clang::FunctionDecl>("metaRel");
    const clang::CallExpr* ce =
        Result.Nodes.getNodeAs<clang::CallExpr>("recursiveCall");
    mrInfo* new_mri = new mrInfo(fd->getQualifiedNameAsString(), ce == nullptr);
    globals::mr_names_list.insert(new_mri);
}

MRGatherer::MRGatherer(clang::ASTContext& ctx)
{
    mr_matcher.addMatcher(
        clang::ast_matchers::functionDecl(
        clang::ast_matchers::allOf(
            clang::ast_matchers::hasAncestor(
            clang::ast_matchers::namespaceDecl(
            clang::ast_matchers::hasName(
            "metalib"))),

            clang::ast_matchers::anyOf(
                clang::ast_matchers::anything(),

                clang::ast_matchers::hasDescendant(
                clang::ast_matchers::callExpr(
                clang::ast_matchers::callee(
                clang::ast_matchers::functionDecl(
                clang::ast_matchers::hasName("placeholder"))))
                    .bind("recursiveCall")))))
            .bind("metaRel"), &mr_logger);

    mr_matcher.matchAST(ctx);
}
