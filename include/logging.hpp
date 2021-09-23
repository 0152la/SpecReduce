#ifndef _LOGGING_HPP
#define _LOGGING_HPP

#include <chrono>
#include <fstream>
#include <ios>
#include <map>

enum REDUCTION_TYPE : size_t;

namespace logging
{

extern bool enable_log;
//extern size_t reductions_attempted;
//extern size_t reductions_applied;
extern std::map<REDUCTION_TYPE, size_t> reductions_attempted;
extern std::map<REDUCTION_TYPE, size_t> reductions_applied;
extern long initial_file_size;
extern long final_file_size;
extern std::time_t time_start;
extern std::time_t time_end;
//extern std::chrono::time_point<std::chrono::steady_clock> time_start;
//extern std::chrono::time_point<std::chrono::steady_clock> time_end;

long getFileSize(std::string);

} // namespace logging

#endif // _LOGGING_HPP
