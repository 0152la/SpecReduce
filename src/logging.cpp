#include "logging.hpp"

namespace logging {

/* Adapted from https://stackoverflow.com/a/2409527 */
long
getFileSize(std::string path)
{
    std::streampos size = 0;
    std::ifstream file(path, std::ios::binary);

    size = file.tellg();
    file.seekg(0, std::ios::end);
    size = file.tellg() - size;
    file.close();

    return size;
}

} // namespace logging
