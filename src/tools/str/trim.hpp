#pragma once

#include <string>

using namespace std;

namespace tools::str {

    string trim(const string& str) {
        // Find the first non-whitespace character from the beginning
        size_t start = str.find_first_not_of(" \t\n\r\f\v");
        
        // If there is no non-whitespace character, return an empty string
        if (start == string::npos) return "";

        // Find the first non-whitespace character from the end
        size_t end = str.find_last_not_of(" \t\n\r\f\v");

        // Return the substring from the first non-whitespace to the last non-whitespace character
        return str.substr(start, end - start + 1);
    }

}

#ifdef TEST

using namespace tools::str;

void test_trim_no_whitespace() {
    string input = "hello";
    string expected = "hello";
    string actual = trim(input);
    assert(actual == expected && "No whitespace to trim");
}

void test_trim_leading_whitespace() {
    string input = "   hello";
    string expected = "hello";
    string actual = trim(input);
    assert(actual == expected && "Leading whitespace");
}

void test_trim_trailing_whitespace() {
    string input = "hello   ";
    string expected = "hello";
    string actual = trim(input);
    assert(actual == expected && "Trailing whitespace");
}

void test_trim_both_leading_and_trailing_whitespace() {
    string input = "   hello   ";
    string expected = "hello";
    string actual = trim(input);
    assert(actual == expected && "Leading and trailing whitespace");
}

void test_trim_only_whitespace() {
    string input = "   \t\n\r\f\v   ";
    string expected = "";
    string actual = trim(input);
    assert(actual == expected && "Only whitespace");
}

void test_trim_empty_string() {
    string input = "";
    string expected = "";
    string actual = trim(input);
    assert(actual == expected && "Empty string");
}

void test_trim_mixed_whitespace() {
    string input = " \t\nhello\r\f\v ";
    string expected = "hello";
    string actual = trim(input);
    assert(actual == expected && "Mixed whitespace");
}

void test_trim_whitespace_in_middle() {
    string input = "hello world";
    string expected = "hello world";
    string actual = trim(input);
    assert(actual == expected && "Whitespace in middle (should not be trimmed)");
}


TEST(test_trim_no_whitespace);
TEST(test_trim_leading_whitespace);
TEST(test_trim_trailing_whitespace);
TEST(test_trim_both_leading_and_trailing_whitespace);
TEST(test_trim_only_whitespace);
TEST(test_trim_empty_string);
TEST(test_trim_mixed_whitespace);
TEST(test_trim_whitespace_in_middle);
#endif
