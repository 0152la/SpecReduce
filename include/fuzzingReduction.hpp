#ifndef _FUZZING_REDUCTION_HPP
#define _FUZZING_REDUCTION_HPP

#include "clang/AST/Decl.h"
#include "clang/AST/RecursiveASTVisitor.h"

#include <set>

class readVarsGatherer : public clang::RecursiveASTVisitor<readVarsGatherer>
{
    public:
        std::set<const clang::VarDecl*> read_vars;

        readVarsGatherer(const clang::VarDecl*);
        bool VisitDeclRefExpr(clang::DeclRefExpr*);
};

class fuzzingTreeNode
{
    public:
        std::set<const clang::VarDecl*> written_vars;
        std::set<const clang::VarDecl*> read_vars;
        std::vector<fuzzingTreeNode*> children;

        fuzzingTreeNode(const clang::VarDecl*, clang::ASTContext&);
        ~fuzzingTreeNode();

        fuzzingTreeNode* getNodeByWrittenVar(const clang::VarDecl*);
};

class fuzzingTree
{
    public:
        fuzzingTreeNode* base;

        fuzzingTree(fuzzingTreeNode* _base) : base(_base) {};
        fuzzingTree(std::vector<const clang::Stmt*>, clang::ASTContext&);
        ~fuzzingTree();

        fuzzingTreeNode* getNodeByWrittenVar(const clang::VarDecl*);
};

#endif // _FUZZING_REDUCTION_HPP

