#include "compileAndExecute.hpp"

void
testExecutor::compileTestCase()
{
    const std::string compile_script_location =
        "/home/sentenced/Documents/Internships/2018_ETH/work/spec_ast/scripts/compile.sh";
    std::stringstream sstream;
    struct stat buffer;
    assert(!stat(compile_script_location.c_str(), &buffer));
    assert(!stat(this->test_path.c_str(), &buffer));
    assert(!stat(this->cmake_file_path.c_str(), &buffer));
    sstream << compile_script_location << " ";
    sstream << this->test_path << " ";
    sstream << this->cmake_file_path << " ";

    int ret_code = std::system(sstream.str().c_str());
    assert(!ret_code);
    this->compiled = true;
}

int
testExecutor::executeTestCase()
{
    assert(compiled);
    const std::string executable_path =
        this->test_path.substr(0, this->test_path.rfind('.'));
    struct stat buffer;
    assert(!stat(executable_path.c_str(), &buffer));

    std::FILE* execute_proc = popen(executable_path.c_str(), "r");
    std::string execute_proc_stdout;
    std::fscanf(execute_proc, "%s", &execute_proc_stdout);
    std::cout << execute_proc_stdout << std::endl;

    int return_code = pclose(execute_proc);
    assert(!errno);
    return return_code;
}

bool
interestingExecutor::runInterestingnessTest(const int expected)
{
    std::string interesting_execute_str =
        this->interesting_script_path + " --logging none " + this->test_path;
    std::cout << interesting_execute_str << std::endl;
    std::FILE* execute_interest_test = popen(interesting_execute_str.c_str(), "r");
    this->return_code = WEXITSTATUS(pclose(execute_interest_test));
    assert(!errno);
    return this->return_code == expected;
}
