#pragma once

#include <string>
#include <functional>
#include <vector>

using namespace std;

namespace tools::testing {

    // Struct to hold test information
    struct Test {
        string name;
        function<void()> run;
        string file;
        int line;
    };

    // Declare the global tests collection
    extern vector<Test> tests;

    // Macro to register a test function
    #undef TEST
    #define TEST(t) \
        struct test_registrar_##t { \
            test_registrar_##t() { \
                tools::testing::tests.push_back({#t, t, __FILE__, __LINE__}); \
            } \
        } test_registrar_instance_##t;

} // namespace tools::testing