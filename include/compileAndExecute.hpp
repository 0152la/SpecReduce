#ifndef _COMPILE_AND_EXECUTE_HPP
#define _COMPILE_AND_EXECUTE_HPP

#include <cassert>
#include <cstdlib>
#include <iostream>
#include <sstream>

#include <stdio.h>
#include <sys/stat.h>

class testExecutor
{
    private:
        const std::string test_path;
        const std::string cmake_file_path;
        bool compiled = false;

    public:
        testExecutor(std::string _test_path, std::string _cmake_path) :
            test_path(_test_path), cmake_file_path(_cmake_path) {};

        void compileTestCase();
        int executeTestCase();
};

class interestingExecutor
{
    private:
        const std::string test_path;
        const std::string interesting_script_path;
        int return_code;

    public:
        interestingExecutor(std::string _test_path, std::string _interest_path) :
            test_path(_test_path), interesting_script_path(_interest_path) {};

        bool runInterestingnessTest(const int = 0);
        int getReturnCode() { return this->return_code; };
};

#endif
