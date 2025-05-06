#pragma once

#include <string>
#include <vector>
#include "str_diff_t.hpp"
#include "str_get_diffs.h"
#include "str_show_diff.h"

using namespace std;

namespace tools::str {

    // Compute and display all differences between two strings
    vector<str_diff_t> str_diffs_show(const string& s1, const string& s2);

} // namespace tools::str

#ifdef TEST
// Test function declarations (in global namespace)
void test_str_diffs_show_identical_strings();
void test_str_diffs_show_added_lines();
void test_str_diffs_show_removed_lines();
void test_str_diffs_show_modified_lines();
void test_str_diffs_show_multiple_blocks();
void test_str_diffs_show_empty_strings();
void test_str_diffs_show_empty_vs_nonempty();
#endif