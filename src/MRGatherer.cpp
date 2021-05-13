#include "MRGatherer.hpp"

mrInfo::mrInfo(std::string _qual_name, bool _base) : is_base(_base)
{
    const std::string delim = "::", check_mr_type = "checks";
    const size_t mr_name_token_count = 4;
    std::string tokenized_name[mr_name_token_count];

    const size_t type_idx = 1;
    const size_t family_idx = 2;
    const size_t check_name_idx = 2, mr_name_idx = 3;

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

    this->type = tokenized_name[type_idx];
    if (!this->type.compare(check_mr_type))
    {
        this->name = tokenized_name[check_name_idx];
    }
    else
    {
        this->family = tokenized_name[family_idx];
        this->name = tokenized_name[mr_name_idx];
    }
}

std::string
mrInfo::getFullName() const
{
    const std::string delim = "::";
    if (this->family.empty())
    {
        return this->type + delim + this->name;
    }
    return this->type + delim + this->family + delim + this->name;
}

mrInfo*
mrInfo::getBaseMR(std::set<mrInfo*>& mr_set) const
{
    for (mrInfo* logged_mr : mr_set)
    {
        if (logged_mr->is_base &&
                !logged_mr->type.compare(this->type) &&
                !logged_mr->family.compare(this->family))
        {
            return logged_mr;
        }
    }
    EMIT_DEBUG_INFO("No base function for MR family " + this->family + " found.", 1);
    assert(false);
}

void
MRLogger::run(const clang::ast_matchers::MatchFinder::MatchResult& Result)
{
    const clang::FunctionDecl* fd =
        Result.Nodes.getNodeAs<clang::FunctionDecl>("metaRel");
    const clang::CallExpr* ce =
        Result.Nodes.getNodeAs<clang::CallExpr>("recursiveCall");
    mrInfo* new_mri = new mrInfo(fd->getQualifiedNameAsString(), ce == nullptr);
    EMIT_DEBUG_INFO("Adding new MR " + fd->getQualifiedNameAsString() +
        " (base " + std::to_string(new_mri->is_base) + ")", 4);
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

            clang::ast_matchers::unless(
            clang::ast_matchers::hasName(
            "placeholder")),

            clang::ast_matchers::anyOf(
                clang::ast_matchers::hasDescendant(
                clang::ast_matchers::callExpr(
                clang::ast_matchers::callee(
                clang::ast_matchers::functionDecl(
                clang::ast_matchers::hasName("placeholder"))))
                    .bind("recursiveCall")),
                clang::ast_matchers::anything())))

        .bind("metaRel"), &mr_logger);
    mr_matcher.matchAST(ctx);
}
