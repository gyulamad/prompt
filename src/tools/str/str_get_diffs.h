#pragma once

#include <string>
#include <vector>
#include "explode.h"
#include "str_diff_t.hpp"

using namespace std;

namespace tools::str {

    // Compute differences between two strings
    vector<str_diff_t> str_get_diffs(const string& s1, const string& s2);

} // namespace tools::str


#ifdef TEST

#include <vector>
#include <string>
#include <iostream> // Included for debug in compare_diff_vectors

#include "../utils/Test.h"     // For TEST macro and assert
#include "str_diff_t.hpp"        // For the diff structure
#include "compare_diff_vectors.h"
// Test function declarations
void test_str_get_diffs_identical();
void test_str_get_diffs_simple_addition();
void test_str_get_diffs_simple_removal();
void test_str_get_diffs_simple_modification();
void test_str_get_diffs_multiple_blocks_like();
void test_str_get_diffs_empty_strings();
void test_str_get_diffs_first_empty();
void test_str_get_diffs_second_empty();
void test_str_get_diffs_trailing_newline_s1();
void test_str_get_diffs_trailing_newline_s2();
#endif