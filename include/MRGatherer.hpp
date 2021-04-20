#ifndef _MR_GATHERER_HPP
#define _MR_GATHERER_HPP

#include "clang/AST/ASTContext.h"
#include "clang/ASTMatchers/ASTMatchers.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"

#include <iostream>

#include "globals.hpp"

struct mrInfo
{
    std::string name;
    std::string family;
    bool is_base;

    mrInfo(std::string, bool);

    std::string getFullName() const { return this->family + "::" + this->name; };

    bool operator<(const mrInfo& other) const {
        return this->getFullName() < other.getFullName(); }
};

class MRLogger : public clang::ast_matchers::MatchFinder::MatchCallback
{
    public:
        virtual void
            run(const clang::ast_matchers::MatchFinder::MatchResult&) override;
};

class MRGatherer
{
    private:
        clang::ast_matchers::MatchFinder mr_matcher;
        MRLogger mr_logger;

    public:
        MRGatherer(clang::ASTContext&);
};

#endif
