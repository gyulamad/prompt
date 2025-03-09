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

// Helper function to capture both cout and cerr output
string test_capture_cout_cerr(function<void()> func) {
    streambuf* original_cout_buffer = cout.rdbuf(); // Store original buffers
    streambuf* original_cerr_buffer = cerr.rdbuf();
    stringstream buffer;
    cout.rdbuf(buffer.rdbuf()); // Redirect cout to buffer
    cerr.rdbuf(cout.rdbuf()); // Redirect cerr to cout's redirected buffer
    func(); // Call the function
    cout.rdbuf(original_cout_buffer); // Restore original cout
    cerr.rdbuf(original_cerr_buffer); // Restore original cerr
    cout.clear(); // Clear any error flags on cout
    cerr.clear(); // Clear any error flags on cerr
    return buffer.str();
}

// Test runner
void run_tests(const string& filter = "") {
    struct failure_s {
        Test test;
        string errmsg;
    };
    vector<failure_s> failures;

    size_t n = 0;
    size_t passed = 0;
    string test_outputs = "";
    string thread_warns = "";
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
            string test_output = test_capture_cout_cerr([&test]() {
                test.run();
            });
            if (!test_output.empty()) 
                test_outputs += 
                    "Test " + ANSI_FMT(ANSI_FMT_T_BOLD ANSI_FMT_C_WHITE, test.name) + "() at " + 
                    test.file + ":" + to_string(test.line) + " outputs:\n" + test_output + "\n";
            
            auto end = chrono::high_resolution_clock::now(); // End timing
            auto duration = chrono::duration_cast<chrono::nanoseconds>(end - start);

            // quick thread check
            size_t threads_count;
            long n = 0;
            long ms = 100;
            long m = 3000;
            string thread_warn = "";
            while ((threads_count = get_threads_count()) > 1) {
                if (thread_warn.empty()) thread_warn = 
                    "Test " + ANSI_FMT(ANSI_FMT_T_BOLD ANSI_FMT_C_WHITE, test.name) + "() at " + 
                    test.file + ":" + to_string(test.line) + " left threads: " + to_string(threads_count) + "\n";
                sleep_ms(ms);
                n += ms;
                if (n > m) break;
            }
            threads_count = get_threads_count();
            if (threads_count > 1)
                throw ERROR(to_string(threads_count-1) + " thread(s) stuck in background after " + to_string(m) + "ms");
            thread_warns += thread_warn;

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

    if (!test_outputs.empty())
        cout << ANSI_FMT(ANSI_FMT_WARNING, "Warning: Some test(s) left outputs:") << endl << test_outputs << flush;
    if (!thread_warns.empty())
        cout << ANSI_FMT(ANSI_FMT_WARNING, "Warning: Some test(s) left running threads after completion:") << endl << thread_warns << flush;

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