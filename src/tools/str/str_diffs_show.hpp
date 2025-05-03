#pragma once

#include <string>
#include <vector>

#include "str_diff_t.hpp"
#include "str_get_diffs.hpp"
#include "str_show_diff.hpp"
#include "../utils/io.hpp"

using namespace std;
using namespace tools::utils;

namespace tools::str {

    // Function to compute and display all differences between two strings
    vector<str_diff_t> str_diffs_show(const string& s1, const string& s2) {
        vector<str_diff_t> diffs = str_get_diffs(s1, s2);
        for (const str_diff_t& diff : diffs) {
            str_show_diff(diff);
        }
        return diffs;
    }
    
}

#ifdef TEST

#include <iostream> // For cout (used by str_show_diff)
#include <vector>
#include <string>
#include <functional> // For function<> in capture_cout_cerr

#include "../utils/ANSI_FMT.hpp" // For ANSI codes used in expected output
#include "compare_diff_vectors.hpp"
// #include "../utils/Test.hpp"     // For TEST macro and assert
// #include "../utils/helpers.hpp"  // For capture_cout_cerr, str_contains, vector_equal (assuming it works for str_diff_t or we implement comparison)
// #include "str_diff_t.hpp"        // For the diff structure

// Assuming str_get_diffs is implicitly included via str_diffs_show.hpp or available
// Assuming str_show_diff is implicitly included via str_diffs_show.hpp or available

using namespace std;
using namespace tools::str;
using namespace tools::utils; // For helpers and ANSI_FMT


// --- Test Cases ---

void test_str_diffs_show_identical_strings() {
    string s1 = "line1\nline2\nline3";
    string s2 = "line1\nline2\nline3";
    vector<str_diff_t> expected_diffs = {}; // Expect no differences
    string expected_output = ""; // Expect no output

    vector<str_diff_t> actual_diffs;
    // LCOV_EXCL_START
    string actual_output = capture_cout_cerr([&]() {
        actual_diffs = str_diffs_show(s1, s2);
    });
    // LCOV_EXCL_STOP

    assert(compare_diff_vectors(actual_diffs, expected_diffs) && "Identical strings: Diffs should be empty");
    assert(actual_output == expected_output && "Identical strings: Output should be empty");
}

void test_str_diffs_show_added_lines() {
    string s1 = "line1\nline3";
    string s2 = "line1\nline2\nline3";
    vector<str_diff_t> expected_diffs = {
        {{2, 2}, {"line2", "line3"}, {"line3"}} // Bound might need adjustment based on str_get_diffs logic
    };
    string expected_output = "changed line(s) 2..2:\n" +
        string(ANSI_FMT_RESET) + ANSI_FMT_C_GREEN + "+ line2" + ANSI_FMT_RESET + "\n" +
        string(ANSI_FMT_RESET) + ANSI_FMT_C_GREEN + "+ line3" + ANSI_FMT_RESET + "\n" +
        string(ANSI_FMT_RESET) + ANSI_FMT_C_RED + "- line3" + ANSI_FMT_RESET + "\n";

    vector<str_diff_t> actual_diffs;
    // LCOV_EXCL_START
    string actual_output = capture_cout_cerr([&]() {
        actual_diffs = str_diffs_show(s1, s2);
    });
    // LCOV_EXCL_STOP

    assert(compare_diff_vectors(actual_diffs, expected_diffs) && "Added lines: Diffs structure mismatch");
    assert(actual_output == expected_output && "Added lines: Output mismatch");
}

void test_str_diffs_show_removed_lines() {
    string s1 = "line1\nline2\nline3";
    string s2 = "line1\nline3";
     vector<str_diff_t> expected_diffs = {
        {{2, 2}, {}, {"line2"}} // Bound might need adjustment
    };
    string expected_output = "changed line(s) 2..2:\n" +
                             string(ANSI_FMT_RESET) + ANSI_FMT_C_RED + "- line2" + ANSI_FMT_RESET + "\n";

    vector<str_diff_t> actual_diffs;
    // LCOV_EXCL_START
    string actual_output = capture_cout_cerr([&]() {
        actual_diffs = str_diffs_show(s1, s2);
    });
    // LCOV_EXCL_STOP

    assert(compare_diff_vectors(actual_diffs, expected_diffs) && "Removed lines: Diffs structure mismatch");
    assert(actual_output == expected_output && "Removed lines: Output mismatch");
}

void test_str_diffs_show_modified_lines() {
    string s1 = "line1\nold_line2\nline3";
    string s2 = "line1\nnew_line2\nline3";
    vector<str_diff_t> expected_diffs = {
        {{2, 3}, {"new_line2", "line3"}, {"old_line2", "line3"}} // Bound might need adjustment
    };
    string expected_output = "changed line(s) 2..3:\n" +
        string(ANSI_FMT_RESET) + ANSI_FMT_C_GREEN + "+ new_line2" + ANSI_FMT_RESET + "\n" +
        string(ANSI_FMT_RESET) + ANSI_FMT_C_GREEN + "+ line3" + ANSI_FMT_RESET + "\n" +
        string(ANSI_FMT_RESET) + ANSI_FMT_C_RED + "- old_line2" + ANSI_FMT_RESET + "\n" + 
        string(ANSI_FMT_RESET) + ANSI_FMT_C_RED + "- line3" + ANSI_FMT_RESET + "\n";

    vector<str_diff_t> actual_diffs;
    // LCOV_EXCL_START
    string actual_output = capture_cout_cerr([&]() {
        actual_diffs = str_diffs_show(s1, s2);
    });
    // LCOV_EXCL_STOP

    assert(compare_diff_vectors(actual_diffs, expected_diffs) && "Modified lines: Diffs structure mismatch");
    assert(actual_output == expected_output && "Modified lines: Output mismatch");
}

void test_str_diffs_show_multiple_blocks() {
    string s1 = "a\nb\nc\nd";
    string s2 = "a\nx\nc\ny";
    vector<str_diff_t> expected_diffs = {
        {{2, 4}, {"x", "c", "y"}, {"b", "c", "d"}}, // Bound might need adjustment
    };
    string expected_output = "changed line(s) 2..4:\n" +
        string(ANSI_FMT_RESET) + ANSI_FMT_C_GREEN + "+ x" + ANSI_FMT_RESET + "\n" +
        string(ANSI_FMT_RESET) + ANSI_FMT_C_GREEN + "+ c" + ANSI_FMT_RESET + "\n" +
        string(ANSI_FMT_RESET) + ANSI_FMT_C_GREEN + "+ y" + ANSI_FMT_RESET + "\n" +
        string(ANSI_FMT_RESET) + ANSI_FMT_C_RED + "- b" + ANSI_FMT_RESET + "\n" +
        string(ANSI_FMT_RESET) + ANSI_FMT_C_RED + "- c" + ANSI_FMT_RESET + "\n" +
        string(ANSI_FMT_RESET) + ANSI_FMT_C_RED + "- d" + ANSI_FMT_RESET + "\n"
    ;

    vector<str_diff_t> actual_diffs;
    // LCOV_EXCL_START
    string actual_output = capture_cout_cerr([&]() {
        actual_diffs = str_diffs_show(s1, s2);
    });
    // LCOV_EXCL_STOP

    assert(compare_diff_vectors(actual_diffs, expected_diffs) && "Multiple blocks: Diffs structure mismatch");
    assert(actual_output == expected_output && "Multiple blocks: Output mismatch");
}

void test_str_diffs_show_empty_strings() {
    string s1 = "";
    string s2 = "";
    vector<str_diff_t> expected_diffs = {};
    string expected_output = "";

    vector<str_diff_t> actual_diffs;
    // LCOV_EXCL_START
    string actual_output = capture_cout_cerr([&]() {
        actual_diffs = str_diffs_show(s1, s2);
    });
    // LCOV_EXCL_STOP

    assert(compare_diff_vectors(actual_diffs, expected_diffs) && "Empty strings: Diffs should be empty");
    assert(actual_output == expected_output && "Empty strings: Output should be empty");
}

void test_str_diffs_show_empty_vs_nonempty() {
    string s1 = "";
    string s2 = "line1\nline2";
     vector<str_diff_t> expected_diffs = {
        {{1, 1}, {"line1", "line2"}, {""}} // Bound might need adjustment
    };
    string expected_output = "changed line(s) 1..1:\n" + // Bound might need adjustment
        string(ANSI_FMT_RESET) + ANSI_FMT_C_GREEN + "+ line1" + ANSI_FMT_RESET + "\n" +
        string(ANSI_FMT_RESET) + ANSI_FMT_C_GREEN + "+ line2" + ANSI_FMT_RESET + "\n";

    vector<str_diff_t> actual_diffs;
    // LCOV_EXCL_START
    string actual_output = capture_cout_cerr([&]() {
        actual_diffs = str_diffs_show(s1, s2);
    });
    // LCOV_EXCL_STOP

    assert(compare_diff_vectors(actual_diffs, expected_diffs) && "Empty vs NonEmpty: Diffs structure mismatch");
    assert(actual_output == expected_output && "Empty vs NonEmpty: Output mismatch");
}


// Register tests
TEST(test_str_diffs_show_identical_strings);
TEST(test_str_diffs_show_added_lines);
TEST(test_str_diffs_show_removed_lines);
TEST(test_str_diffs_show_modified_lines);
TEST(test_str_diffs_show_multiple_blocks);
TEST(test_str_diffs_show_empty_strings);
TEST(test_str_diffs_show_empty_vs_nonempty);


#endif // TEST
