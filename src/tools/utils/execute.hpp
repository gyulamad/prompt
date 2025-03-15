#pragma once

#include <string>
#include "ERROR.hpp"

using namespace std;

namespace tools::utils {

    static string execute(const char* cmd) {
        string result = "";
        char buffer[128];
        FILE* pipe = popen(cmd, "r");
        if (!pipe) throw ERROR("popen() failed!");
        while (!feof(pipe))
            if (fgets(buffer, 128, pipe) != nullptr) result += buffer;
        int status = pclose(pipe);
        if (status == -1) throw ERROR("pclose() failed!");
        return result;
    }

}

#ifdef TEST

#include "Test.hpp"

using namespace tools::utils;

void test_execute_successful_output() {
    string result = execute("echo Hello");
    // On some systems, echo appends a newline
    assert((result == "Hello\n" || result == "Hello") && "execute should capture echo output");
}

void test_execute_empty_output() {
    string result = execute("true"); // POSIX command that does nothing
    assert(result.empty() && "execute should return empty string for no output");
}

void test_execute_multiline_output() {
    // Use a portable command; 'ls -d .' on Unix, 'dir' on Windows might need adjustment
    string result = execute("printf 'Line1\nLine2\n'");
    assert(result == "Line1\nLine2\n" && "execute should concatenate multi-line output");
}

// Register tests
TEST(test_execute_successful_output);
TEST(test_execute_empty_output);
TEST(test_execute_multiline_output);
#endif