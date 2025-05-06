#pragma once

#include <vector>
#include "str_diff_t.hpp"  // For the str_diff_t structure

using namespace std;

namespace tools::str {

    // Compare two vectors of str_diff_t for equality
    bool compare_diff_vectors(const vector<str_diff_t>& actual, const vector<str_diff_t>& expected);

}


#ifdef TEST
// Test function declarations
void test_compare_diff_vectors_identical();
void test_compare_diff_vectors_different_sizes();
void test_compare_diff_vectors_different_bounds();
void test_compare_diff_vectors_different_added();
void test_compare_diff_vectors_different_removed();
void test_compare_diff_vectors_empty();
#endif