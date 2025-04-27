#pragma once

#include <string>
#include <functional>
#include <vector>

using namespace std;

namespace tools::utils {

    struct Test {
        string name;
        function<void()> run;
        string file;
        int line;
    };

    vector<Test> tests;

#undef TEST
#define TEST(t) \
    struct test_registrar_##t { \
        test_registrar_##t() { \
            tests.push_back({#t, t, __FILE__, __LINE__}); \
        } \
    } test_registrar_instance_##t; \

}