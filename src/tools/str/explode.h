#pragma once

#include <string>
#include <vector>
#include "../utils/ERROR.h"

using namespace std;

namespace tools::str {

    // Split a string into a vector of strings based on a delimiter
    vector<string> explode(const string& delimiter, const string& str);

} // namespace tools::str

#ifdef TEST
// Test function declarations (in global namespace)
void test_explode_basic();
void test_explode_no_delimiter();
void test_explode_start_with_delimiter();
void test_explode_end_with_delimiter();
void test_explode_consecutive_delimiters();
void test_explode_only_delimiters();
void test_explode_empty_input();
void test_explode_invalid_delimiter();
#endif