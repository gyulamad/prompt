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
#include <vector>
#include <iostream>
#include <functional>
#include <chrono>

using namespace std;

 // TODO it should goes to the namespace tools::testing
#ifdef TEST

/** use this macro:
TEST macro -> register a test, TEST_SKIP macro -> goes inside the test an skips it. eg:
void test_foo() {
    TEST_SKIP("not runs"); // throws an exception in the background
    ...
} 
*/
#define TEST_SKIP(...) { \
    const char* msg = #__VA_ARGS__; \
    if (msg[0] == '\0') msg = "skip reason not specified"; \
    cerr << "Test skipped '" + string(msg) + "': " << ANSI_FMT_CALL(__FUNC__, __FILE__, __LINE__) << endl; return; \
}

#include "assert.hpp"
#include "Test.hpp"
#include "Stopper.hpp"

#define TEST_SIGN_NONE ANSI_FMT_RESET "[ ]"
#define TEST_SIGN_PASS ANSI_FMT_RESET "[" ANSI_FMT_SUCCESS "✔" ANSI_FMT_RESET "]"
#define TEST_SIGN_WARN ANSI_FMT_RESET "[" ANSI_FMT_WARNING "!" ANSI_FMT_RESET "]"
#define TEST_SIGN_FAIL ANSI_FMT_RESET "[" ANSI_FMT_ERROR "✖" ANSI_FMT_RESET "]"


// because json, ggml (and maybe other) lib(S) overrides assert otherwise
#define TEST_ASSERT_OVERRIDE(x) { if (!(x)) cerr << ERROR("TEST_ASSERT_OVERRIDE FAILED: "#x).what() << endl; exit(1); }
// #define TEST_ASSERT_OVERRIDE(x) { if (!(x)) cerr << "TEST_ASSERT_OVERRIDE FAILED: "#x << endl; exit(1); }
// #define TEST_ASSERT_OVERRIDE(x) assert(x)
// #define GGML_ASSERT(x) TEST_ASSERT_OVERRIDE(x)
// #define JSON_ASSERT(x) TEST_ASSERT_OVERRIDE(x)



#include "ANSI_FMT.hpp"
#include "ERROR.hpp"
#include "system.hpp"

using namespace tools::utils;


string for_test_implode(const string& delimiter, const vector<string>& elements) {
    ostringstream oss;
    for (size_t i = 0; i < elements.size(); ++i) {
        if (i != 0) oss << delimiter;
        oss << elements[i];
    }
    return oss.str();
}

// Test runner
inline int run_tests(const vector<string>& filters = {}, bool failure_throws = false, bool failure_exits = false) {
    Stopper stopper;
    stopper.start();

    struct failure_s {
        Test test;
        string errmsg;
    };
    vector<failure_s> failures;

#ifdef TEST_VERBOSE
    size_t n = 0;
#endif
    size_t passed = 0;
    size_t warned = 0;
    string test_outputs = "";
    string thread_warns = "";
    for (const auto& test : tests) { 

        // skip tests where the filename or the test name both are not containing the filter
        // (or do all if the filter is empty)        
        bool skip = false;
        if (!filters.empty()) {
            skip = true;
            for (const string& filter: filters) {
                if (!filter.empty() &&
                    (
                        test.file.find(filter) != string::npos ||
                        test.name.find(filter) != string::npos
                    )
                ) {
                    skip = false;
                    break;
                }
            }
        }
        if (skip) continue;
        
#ifdef TEST_VERBOSE
        cout 
            << TEST_SIGN_NONE " [.............] Testing: " 
            << to_string(tests.size()) << "/" << ++n << " " 
            << ANSI_FMT_CALL(test.name, test.file, test.line) << flush;
#else
        cout << "." << flush;
#endif
            
        string tick_or_warn = TEST_SIGN_PASS;
        auto start = chrono::high_resolution_clock::now(); // Start timing TODO: use Stopper class
        try {
            string test_output = capture_cout_cerr([&test]() {
                test.run();
            });
            if (!test_output.empty()) 
                test_outputs += 
                    TEST_SIGN_WARN " Test " + 
                    ANSI_FMT_CALL(test.name, test.file, test.line)
                    + "\noutput:\n" + test_output + "\n";
            
            auto end = chrono::high_resolution_clock::now(); // End timing
            auto duration = chrono::duration_cast<chrono::nanoseconds>(end - start);


            // quick thread check
            size_t threads_count;
            long n = 0;
            long ms = 100; // steps time to re-check stuck threads
            long m = 3000; // max tolerance we could wait before a thread considered stuck
            long t = 1000; // timewindow for the OS to manage the threads
            string thread_warn = "";
            while ((threads_count = get_threads_count()) > 1) {
                if (thread_warn.empty()) thread_warn = 
                    TEST_SIGN_WARN " Test " + ANSI_FMT_CALL(test.name, test.file, test.line) + 
                    " left threads: " + to_string(threads_count) + "\n";
                sleep_ms(ms);
                n += ms;
                if (n > m) break;
            }
            if (n < t) thread_warn = ""; // suppress the thread warning if it less that the time window that OS need for bookkeeping
            threads_count = get_threads_count();
            if (threads_count > 1)
                throw ERROR(to_string(threads_count-1) + " thread(s) stuck in background after " + to_string(m) + "ms");
            thread_warns += thread_warn;

            if (!test_output.empty() || !thread_warn.empty()) {
                tick_or_warn = TEST_SIGN_WARN;
                warned++;
            }
#ifdef TEST_VERBOSE
            cout << "\r" << tick_or_warn << " [" << duration.count() << "ns" << endl; // Show time
#endif
            passed++;
        } catch (exception &e) {
            auto end = chrono::high_resolution_clock::now(); // End timing
            auto duration = chrono::duration_cast<chrono::nanoseconds>(end - start);
#ifdef TEST_VERBOSE
            cout << endl;
#else
            cout << "\r" << TEST_SIGN_FAIL " [" << duration.count() << "ns" << endl; // Show time
#endif
            string errmsg = 
                ANSI_FMT(ANSI_FMT_ERROR, "Error: ") +
                ANSI_FMT(ANSI_FMT_HIGHLIGHT, e.what());
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

#ifdef TEST_VERBOSE
    cout << endl << "=====[ TESTING FINISHED ]=====" << endl;
    cout << "Elapsed " << stopper.stop() << "ms" << endl;
#else
    cout << endl;
#endif

    if (!test_outputs.empty())
        cout << ANSI_FMT(ANSI_FMT_WARNING, "Warning: Some test(s) left outputs:") << endl << test_outputs << flush;
    if (!thread_warns.empty())
        cout << ANSI_FMT(ANSI_FMT_WARNING, "Warning: Some test(s) left running threads after completion:") << endl << thread_warns << flush;

    // Summary message
    if (failures.size() != 0) {
        string errmsg = "";
        errmsg += ANSI_FMT(ANSI_FMT_ERROR, to_string(failures.size()) + "/" + to_string(tests.size()) + " test(s) failed:") + "\n";
        for (const failure_s& failure: failures)
            errmsg += ANSI_FMT_CALL(failure.test.name, failure.test.file, failure.test.line)
            + "\n" + failure.errmsg + "\n";
        cout << errmsg << flush;
        if (failure_throws) throw ERROR(errmsg);
        if (failure_exits) exit(1);
#ifdef TEST_FAILURE_THROWS
            throw ERROR(errmsg);
#endif
#ifdef TEST_FAILURE_EXITS
            exit(1);
#endif            
    } else {
        if (passed == tests.size()) {
            cout 
                << ANSI_FMT(ANSI_FMT_SUCCESS, "All " + to_string(tests.size()) + " tests passed") 
                << (warned ? ANSI_FMT(ANSI_FMT_WARNING, " - but there are " + to_string(warned) + " warning(s)!") : "")
                << endl;
        } else
            cout << ANSI_FMT(ANSI_FMT_SUCCESS, to_string(tests.size()) + "/" + to_string(passed) + " tests passed") << endl;
    }
    if (failures.size() + passed != tests.size())
        cout << ansi_fmt(ANSI_FMT_WARNING, 
                to_string(tests.size()) + "/" + to_string(tests.size() - passed) 
                + " tests are skipped, filtered by keyword(s): '" 
                + for_test_implode("', '", filters) + "'"
            ) 
            << endl;

    size_t result = tests.size() - passed;
#ifdef TEST_ONLY
    exit(result);
#else
    return (int)result;
#endif
}

#else
inline int run_tests(const vector<string>& = {}, bool = false, bool = false) { return 0; };
#endif
