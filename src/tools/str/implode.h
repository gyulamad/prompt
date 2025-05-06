#pragma once

#include <string>
#include <vector>

using namespace std;

namespace tools::str {

    // Join a vector of strings with a delimiter
    string implode(const string& delimiter, const vector<string>& elements);

} // namespace tools::str

#ifdef TEST
// Test function declarations (in global namespace)
void test_implode_basic();
void test_implode_single_element();
void test_implode_empty_elements();
void test_implode_empty_delimiter();
void test_implode_empty_vector();
void test_implode_mixed_elements();
#endif