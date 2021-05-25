#include "helperFunctions.hpp"

void
EMIT_DEBUG_INFO(const std::string debug_message, size_t priority)
{
    if (priority <= globals::debug_level)
    {
        if (priority == 1)
        {
            std::cout << "\033[1m\033[31m" << "DEBUG] " << "\033[m";
        }
        else
        {
            std::cout << "DEBUG] ";
        }
        std::cout << debug_message << std::endl;
    }
}

void
ERROR_CHECK(std::error_code ec)
{
    if (ec)
    {
        std::cout << std::flush << ec << std::endl;
        std::cout << ec.message() << std::endl;
        exit(1);
    }
}

const mrInfo*
checkFunctionIsMRCall(const clang::FunctionDecl* fd)
{
    EMIT_DEBUG_INFO("Checking function " + fd->getNameAsString() + " is MR call.", 4);
    std::string fd_name = fd->getQualifiedNameAsString();

    // Special case for direct MR calls, not transplanted ones
    if (fd_name.find("metalib::") == 0)
    {
        for (const mrInfo* mr_info : globals::mr_names_list)
        {
            std::string mr_name = mr_info->qual_name;
            if (!fd_name.compare(mr_name))
            {
                EMIT_DEBUG_INFO("Found MR " + mr_name + " for call " + fd_name, 4);
                return mr_info;
            }
        }
        return nullptr;
    }

    // Must check if the function call is of the unqualified form <name>_<id>,
    // where <name> is a declared MR and <id> is a unique digit identifier
    fd_name = fd->getNameAsString();
    std::string::reverse_iterator r_it = fd_name.rbegin();
    size_t delim_count = 0, expected_count = 2, char_count = 0;
    while (delim_count < expected_count && r_it != fd_name.rend())
    {
        if (*r_it == globals::delimiter)
        {
            delim_count += 1;
        }
        r_it = std::next(r_it);
        char_count += 1;
    }
    fd_name = fd_name.substr(0, fd_name.size() - char_count);
    if (!fd_name.empty())
    {
        for (const mrInfo* mr_info : globals::mr_names_list)
        {
            if (!mr_info->family.compare("checks"))
            {
                continue;
            }
            std::string mr_name = mr_info->name;
            if (mr_name.find(fd_name) != std::string::npos &&
                    mr_name.find(fd_name) == mr_name.size() - fd_name.size())
            {
                EMIT_DEBUG_INFO("Found MR " + mr_name + " for call " + fd_name, 4);
                return mr_info;
            }
        }
    }
    return nullptr;
}
