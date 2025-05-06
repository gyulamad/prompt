#pragma once

#include <string>

using namespace std;

namespace tools::str {

    // Truncate a string to a maximum length, appending a suffix if needed
    string str_cut_end(const string& s, size_t maxch = 300, const string& append = "...");

} // namespace tools::str

#ifdef TEST
// Test function declarations (in global namespace)
void test_str_cut_end_when_string_is_short();
void test_str_cut_end_when_string_is_exact_length();
void test_str_cut_end_when_string_needs_truncation();
void test_str_cut_end_when_custom_append_is_used();
void test_str_cut_end_when_string_is_empty();
void test_str_cut_end_when_maxch_is_less_than_append_length();
#endif