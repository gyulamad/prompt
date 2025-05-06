#pragma once

#include <string>

using namespace std;

namespace tools::str {

    // Check if a string starts with a prefix
    bool str_starts_with(const string& str, const string& prefix);

} // namespace tools::str

#ifdef TEST
// Test function declarations (in global namespace)
void test_str_starts_with_basic();
void test_str_starts_with_empty_prefix();
void test_str_starts_with_empty_string();
void test_str_starts_with_prefix_longer_than_string();
void test_str_starts_with_no_match();
void test_str_starts_with_case_sensitive();
#endif