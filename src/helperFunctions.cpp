#include "helperFunctions.hpp"

void
EMIT_DEBUG_INFO(const std::string debug_message)
{
    //if (debug_info)
    //{
        std::cout << "DEBUG]  " << debug_message << std::endl;
    //}
}

const mrInfo*
checkFunctionIsMRCall(const clang::FunctionDecl* fd)
{
    //EMIT_DEBUG_INFO("Checking function " + fd->getNameAsString() + " is MR call.");
    std::string fd_name = fd->getNameAsString();
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
                return mr_info;
            }
        }
    }
    return nullptr;
}
