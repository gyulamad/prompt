#include "str_cut_end.h"

using namespace std;

namespace tools::str {

    string str_cut_end(const string& s, size_t maxch, const string& append) {
        // Check if the string is already shorter than or equal to the limit
        if (s.length() <= maxch) return s;

        // Handle the case where maxch is smaller than or equal to the length of append
        if (maxch <= append.length()) return append;

        // Truncate the string and append the suffix
        return s.substr(0, maxch - append.length()) + append;
    }

} // namespace tools::str

#ifdef TEST

#include "../utils/Test.h"
#include "../utils/assert.hpp"

using namespace tools::str;

void test_str_cut_end_when_string_is_short() {
    string input = "hello";
    string expected = "hello";
    string actual = str_cut_end(input, 10);
    assert(actual == expected && "test_str_cut_end_when_string_is_short failed");
}

void test_str_cut_end_when_string_is_exact_length() {
    string input = "abcdefghij";
    string expected = "abcdefghij";
    string actual = str_cut_end(input, 10);
    assert(actual == expected && "test_str_cut_end_when_string_is_exact_length failed");
}

void test_str_cut_end_when_string_needs_truncation() {
    string input = "abcdefghijklmnopqrstuvwxyz";
    string expected = "abcdefghijkl...";
    string actual = str_cut_end(input, 15);
    assert(actual == expected && "test_str_cut_end_when_string_needs_truncation failed");
}

void test_str_cut_end_when_custom_append_is_used() {
    string input = "abcdefghijklmnopqrstuvwxyz";
    string expected = "abcdefghijklm!!";
    string actual = str_cut_end(input, 15, "!!");
    assert(actual == expected && "test_str_cut_end_when_custom_append_is_used failed");
}

void test_str_cut_end_when_string_is_empty() {
    string input = "";
    string expected = "";
    string actual = str_cut_end(input, 10);
    assert(actual == expected && "test_str_cut_end_when_string_is_empty failed");
}

void test_str_cut_end_when_maxch_is_less_than_append_length() {
    string input = "abcdefghijklmnopqrstuvwxyz";
    string expected = "...";
    string actual = str_cut_end(input, 2);
    assert(actual == expected && "test_str_cut_end_when_maxch_is_less_than_append_length failed");
}

// Test registrations
TEST(test_str_cut_end_when_string_is_short);
TEST(test_str_cut_end_when_string_is_exact_length);
TEST(test_str_cut_end_when_string_needs_truncation);
TEST(test_str_cut_end_when_custom_append_is_used);
TEST(test_str_cut_end_when_string_is_empty);
TEST(test_str_cut_end_when_maxch_is_less_than_append_length);

#endif // TEST