#include "fuzzingReduction.hpp"

readVarsGatherer::readVarsGatherer(const clang::VarDecl* input_vd)
{
    this->TraverseDecl(const_cast<clang::VarDecl*>(input_vd));
}

bool
readVarsGatherer::VisitDeclRefExpr(clang::DeclRefExpr* dre)
{
    if (clang::VarDecl* vd = llvm::dyn_cast<clang::VarDecl>(dre->getDecl()))
    {
        this->read_vars.insert(vd);
    }
    clang::RecursiveASTVisitor<readVarsGatherer>::VisitDeclRefExpr(dre);
    return true;
}

fuzzingTreeNode::fuzzingTreeNode(const clang::VarDecl* input_vd, clang::ASTContext& ctx)
{
    this->written_vars.insert(input_vd);
    readVarsGatherer rvg(input_vd);
    this->read_vars = rvg.read_vars;
}

fuzzingTreeNode::~fuzzingTreeNode()
{
    for (fuzzingTreeNode* child : this->children)
    {
        delete child;
    }
}

fuzzingTreeNode*
fuzzingTreeNode::getNodeByWrittenVar(const clang::VarDecl* write_var)
{
    std::set<const clang::VarDecl*>::const_iterator search_vd =
        this->read_vars.find(write_var);

    if (search_vd != this->read_vars.end())
    {
        return this;
    }
    for (fuzzingTreeNode* child : this->children)
    {
        if (fuzzingTreeNode* found = child->getNodeByWrittenVar(write_var))
        {
            return found;
        }
    }
    return nullptr;
}

/**/

fuzzingTree::fuzzingTree(std::vector<const clang::Stmt*> input, clang::ASTContext& ctx)
{
    auto input_it = input.rbegin();
    const clang::DeclStmt* ds = llvm::dyn_cast<clang::DeclStmt>(*input_it);
    assert(ds);
    assert(ds->isSingleDecl());
    const clang::VarDecl* vd = llvm::dyn_cast<clang::VarDecl>(ds->getSingleDecl());
    assert(vd);
    this->base = new fuzzingTreeNode(vd, ctx);
    input_it += 1;
    for (; input_it != input.rend(); ++input_it)
    {
        ds = llvm::dyn_cast<clang::DeclStmt>(*input_it);
        if (!ds)
        {
            continue;
        }
        assert(ds->isSingleDecl());
        vd = llvm::dyn_cast<clang::VarDecl>(ds->getSingleDecl());
        assert(vd);
        this->getNodeByWrittenVar(vd)->children.push_back(new fuzzingTreeNode(vd, ctx));
    }
}

fuzzingTree::~fuzzingTree()
{
    delete this->base;
}

fuzzingTreeNode*
fuzzingTree::getNodeByWrittenVar(const clang::VarDecl* write_var)
{
    return this->base->getNodeByWrittenVar(write_var);
}

/**/


