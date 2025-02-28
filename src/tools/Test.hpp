#pragma once

// build with -DTEST: 
// note: add -DTEST_CASSERT (optional) // <-- TODO
// note: add -DTEST_FAILURE_EXITS (optional)
// note: add -DTEST_FAILURE_THROWS (optional)
// add to the main():
// int main(int argc, char *argv[]) {
//     run_tests();
//     ...
// g++ your_program.cpp -DTEST -o your_program

#ifdef TEST
#define JSON_ASSERT // because json lib overrides assert otherwise

#include <iostream>
#include <functional>
#include <vector>
#include <string>
#include <chrono>

#include "ANSI_FMT.hpp"
#include "ERROR.hpp"

using namespace std;

#define assert(expr) if (!(expr)) throw ERROR("Assert failed: "#expr);

struct Test {
    string name;
    function<void()> run;
    string file;
    int line;
};

vector<Test> tests;

#undef TEST
#define TEST(t) \
    /* void t(); */\
    struct test_registrar_##t { \
        test_registrar_##t() { \
            tests.push_back({#t, t, __FILE__, __LINE__}); \
        } \
    } test_registrar_instance_##t; \
    /* void t() */

// Test runner
void run_tests() {
    struct failure_s {
        Test test;
        string errmsg;
    };
    vector<failure_s> failures;

    size_t n = 0;
    size_t failed = 0;
    for (const auto& test : tests) {
        cout 
            << "[ ] [.............] Testing: " 
            << to_string(tests.size()) << "/" << ++n << " " 
            << ANSI_FMT(ANSI_FMT_T_BOLD ANSI_FMT_C_WHITE, test.name) << "() at " 
            << test.file << ":" << test.line << flush;
        auto start = chrono::high_resolution_clock::now(); // Start timing
        try {
            test.run();
            auto end = chrono::high_resolution_clock::now(); // End timing
            auto duration = chrono::duration_cast<chrono::nanoseconds>(end - start);
            cout << "\r[" << ANSI_FMT(ANSI_FMT_T_BOLD ANSI_FMT_C_GREEN, "✓") << "] ";
            cout << "[" << duration.count() << "ns" << endl; // Show time
        } catch (exception &e) {
            auto end = chrono::high_resolution_clock::now(); // End timing
            auto duration = chrono::duration_cast<chrono::nanoseconds>(end - start);
            cout << "\r[" << ANSI_FMT(ANSI_FMT_T_BOLD ANSI_FMT_C_RED, "x") << "] ";
            cout << "[" << duration.count() << "ns" << endl; // Show time
            string errmsg = 
                ANSI_FMT(ANSI_FMT_T_BOLD ANSI_FMT_C_RED, "Error: ") +
                ANSI_FMT(ANSI_FMT_T_BOLD ANSI_FMT_C_WHITE, e.what());
            cout << errmsg << endl;
            failed++;
#ifdef TEST_FAILURE_EXITS
            exit(1);
#endif
#ifdef TEST_FAILURE_THROWS
            throw e;
#endif
            failures.push_back({ test, errmsg });
        }
    }

    // Summary message
    if (failed > 0) {
        cout << ANSI_FMT(ANSI_FMT_T_BOLD ANSI_FMT_C_RED, to_string(failed) + "/" + to_string(tests.size()) + " test(s) failed:") << endl;
        for (const failure_s& failure: failures)
            cout << failure.test.name << "() at " << failure.test.file << ":" << failure.test.line << endl << failure.errmsg << endl;
    } else {
        cout << ANSI_FMT(ANSI_FMT_T_BOLD ANSI_FMT_C_GREEN, "All " + to_string(tests.size()) + " tests passed") << endl;
    }
}

#else
// #define TEST(t) 
inline void run_tests() {};
#endif