#pragma once

#include <string>

using namespace std;

namespace tools::str {

    string str_cut_begin(const string& s, size_t maxch = 300, const string& prepend = "...") {
        // Check if the string is already shorter than or equal to the limit
        if (s.length() <= maxch) return s;

        // Handle the case where maxch is smaller than the length of prepend
        if (maxch <= prepend.length()) return prepend;
        
        // Truncate the string from the beginning and prepend the prefix
        return prepend + s.substr(s.length() - (maxch - prepend.length()));
    }
    
}

#ifdef TEST

using namespace tools::str;


void test_str_cut_begin_when_string_is_short() {
    string input = "hello";
    string expected = "hello";
    string actual = str_cut_begin(input, 10);
    assert(actual == expected && "test_str_cut_begin_when_string_is_short failed");
}

void test_str_cut_begin_when_string_is_exact_length() {
    string input = "abcdefghij";
    string expected = "abcdefghij";
    string actual = str_cut_begin(input, 10);
    assert(actual == expected && "test_str_cut_begin_when_string_is_exact_length failed");
}

void test_str_cut_begin_when_string_needs_truncation() {
    string input = "abcdefghijklmnopqrstuvwxyz";
    string expected = "...opqrstuvwxyz";
    string actual = str_cut_begin(input, 15);
    assert(actual == expected && "test_str_cut_begin_when_string_needs_truncation failed");
}

void test_str_cut_begin_when_custom_prepend_is_used() {
    string input = "abcdefghijklmnopqrstuvwxyz";
    string expected = "!!nopqrstuvwxyz";
    string actual = str_cut_begin(input, 15, "!!");
    assert(actual == expected && "test_str_cut_begin_when_custom_prepend_is_used failed");
}

void test_str_cut_begin_when_string_is_empty() {
    string input = "";
    string expected = "";
    string actual = str_cut_begin(input, 10);
    assert(actual == expected && "test_str_cut_begin_when_string_is_empty failed");
}

void test_str_cut_begin_when_maxch_is_less_than_prepend_length() {
    string input = "abcdefghijklmnopqrstuvwxyz";
    string expected = "...";
    string actual = str_cut_begin(input, 2);
    assert(actual == expected && "test_str_cut_begin_when_maxch_is_less_than_prepend_length failed");
}


TEST(test_str_cut_begin_when_string_is_short);
TEST(test_str_cut_begin_when_string_is_exact_length);
TEST(test_str_cut_begin_when_string_needs_truncation);
TEST(test_str_cut_begin_when_custom_prepend_is_used);
TEST(test_str_cut_begin_when_string_is_empty);
TEST(test_str_cut_begin_when_maxch_is_less_than_prepend_length);
#endif
