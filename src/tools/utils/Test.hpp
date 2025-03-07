#pragma once

// build with -DTEST: 
// note: add -DTEST_CASSERT (optional: uses the original assert() function)
// note: add -DTEST_FAILURE_DIES_FAST (optional: stop testing soon after the first failure) 
// note: add -DTEST_FAILURE_EXITS (optional: exits when at least one of the tests failed)
// note: add -DTEST_FAILURE_THROWS (optional: throws when at least one of the tests failed)
// note: add -DTEST_ONLY (optional: exits after tests are ran, no main program execution)
// add to the main():
// int main(int argc, char *argv[]) {
//     run_tests();
//     ...
// g++ your_program.cpp -DTEST -o your_program

#include <string>
using namespace std;

#ifdef TEST
#define JSON_ASSERT // because json lib overrides assert otherwise

#include <iostream>
#include <functional>
#include <vector>
#include <chrono>

#include "ANSI_FMT.hpp"
#include "ERROR.hpp"
#include "system.hpp"

using namespace tools::utils;

#ifdef TEST_CASSERT
#include <cassert>
#else
#define assert(expr) if (!(expr)) throw ERROR("Assert failed: "#expr);
#endif

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
void run_tests(const string& filter = "") {
    struct failure_s {
        Test test;
        string errmsg;
    };
    vector<failure_s> failures;

    size_t n = 0;
    size_t passed = 0;
    for (const auto& test : tests) { 
        // skip tests where the filename or the test name both are not containing the filter
        // (or do all if the filter is empty)
        if (!filter.empty() &&
            !(
                test.file.find(filter) != string::npos ||
                test.name.find(filter) != string::npos
            )
        ) continue;
        
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

            // quick thread check
            size_t threads_count;
            long n = 0;
            long ms = 100;
            long m = 3000;
            while ((threads_count = get_threads_count()) > 1) {
                sleep_ms(ms);
                n += ms;
                if (n > m) break;
            }
            threads_count = get_threads_count();
            if (threads_count > 1)
                throw ERROR(to_string(threads_count-1) + " thread(s) stuck in background after " + to_string(m) + "ms");

            cout << "\r[" << ANSI_FMT(ANSI_FMT_T_BOLD ANSI_FMT_C_GREEN, "✓") << "] ";
            cout << "[" << duration.count() << "ns" << endl; // Show time
            passed++;
        } catch (exception &e) {
            auto end = chrono::high_resolution_clock::now(); // End timing
            auto duration = chrono::duration_cast<chrono::nanoseconds>(end - start);
            cout << "\r[" << ANSI_FMT(ANSI_FMT_T_BOLD ANSI_FMT_C_RED, "x") << "] ";
            cout << "[" << duration.count() << "ns" << endl; // Show time
            string errmsg = 
                ANSI_FMT(ANSI_FMT_T_BOLD ANSI_FMT_C_RED, "Error: ") +
                ANSI_FMT(ANSI_FMT_T_BOLD ANSI_FMT_C_WHITE, e.what());
            cout << errmsg << endl;
            failures.push_back({ test, errmsg });
#ifdef TEST_FAILURE_DIES_FAST
#ifdef TEST_FAILURE_THROWS
            throw ERROR(errmsg);
#endif
#ifdef TEST_FAILURE_EXITS
            exit(1);
#endif
#endif
        }
    }

    // Summary message
    if (failures.size() != 0) {
        string errmsg = "";
        errmsg += ANSI_FMT(ANSI_FMT_T_BOLD ANSI_FMT_C_RED, to_string(failures.size()) + "/" + to_string(tests.size()) + " test(s) failed:") + "\n";
        for (const failure_s& failure: failures)
            errmsg += ANSI_FMT(ANSI_FMT_T_BOLD ANSI_FMT_C_WHITE, failure.test.name) + "() at " + failure.test.file + ":" + to_string(failure.test.line) + "\n" + failure.errmsg + "\n";
        cout << errmsg << flush;
#ifdef TEST_FAILURE_THROWS
            throw ERROR(errmsg);
#endif
#ifdef TEST_FAILURE_EXITS
            exit(1);
#endif            
    } else {
        if (passed == tests.size())
            cout << ANSI_FMT(ANSI_FMT_T_BOLD ANSI_FMT_C_GREEN, "All " + to_string(tests.size()) + " tests passed") << endl;
        else
            cout << ANSI_FMT(ANSI_FMT_T_BOLD ANSI_FMT_C_GREEN, to_string(tests.size()) + "/" + to_string(passed) + " tests passed") << endl;
    }
    if (failures.size() + passed != tests.size())
        cout << ANSI_FMT(ANSI_FMT_T_BOLD ANSI_FMT_C_YELLOW, to_string(tests.size()) + "/" + to_string(tests.size() - passed) + " tests are skipped, filtered by keyword: '" + filter + "'") << endl;

#ifdef TEST_ONLY
    exit(passed != tests.size());
#endif
}

#else
inline void run_tests(const string& = "") {};
#endif