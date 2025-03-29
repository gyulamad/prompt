#pragma once

#include <string>

using namespace std;

namespace tools::str {

    string escape(const string& input, const string& chars = "\\$\"'`\n\r\t", const string& esc = "\\") {
        string result;
        for (size_t i = 0; i < input.size(); ++i) {
            char c = input[i];
            bool needs_escape = (chars.find(c) != string::npos);
            bool is_escaped = false;

            // Check if the previous character in the result is an escape
            if (!result.empty() && result.back() == esc[0]) {
                // Count consecutive escape characters from the end of the result
                size_t escape_count = 0;
                for (ssize_t j = result.size() - 1; j >= 0 && result[j] == esc[0]; --j) escape_count++;

                // Odd number of escapes means this character is already escaped
                is_escaped = (escape_count % 2 == 1);
            }

            if (needs_escape && !is_escaped) result += esc;
            result += c;
        }
        return result;
    }
    
}

#ifdef TEST

using namespace tools::str;

void test_escape_empty_input() {
    string input = "";
    string expected = "";
    string actual = escape(input);
    assert(actual == expected && "Empty input");
}

void test_escape_no_special_chars() {
    string input = "hello world";
    string expected = "hello world";
    string actual = escape(input);
    assert(actual == expected && "No special chars");
}

void test_escape_single_char() {
    string input = "$";
    string expected = "\\$";
    string actual = escape(input);
    assert(actual == expected && "Single char");
}

void test_escape_already_escaped() {
    string input = "\\\\$";
    string expected = "\\\\\\\\\\$";
    string actual = escape(input);
    assert(actual == expected && "Already escaped characters");
}

void test_escape_no_chars() {
    string input = "hello";
    string expected = "hello";
    string actual = escape(input);
    assert(actual == expected && "No characters to escape");
}

void test_escape_custom_chars() {
    string input = "a%b&c";
    string expected = "a\\%b\\&c";
    string actual = escape(input, "%&");
    assert(actual == expected && "Custom characters to escape");
}

void test_escape_mixed_content() {
    string input = "hello $world\\ \" \' `";
    string expected = "hello \\$world\\\\ \\\" \\' \\`";
    string actual = escape(input);
    assert(actual == expected && "Mixed content");
}

void test_escape_custom_chars2() {
    string input = "abc123";
    string chars = "123";
    string expected = "abc\\1\\2\\3";
    string actual = escape(input, chars);
    assert(actual == expected && "Custom chars 2");
}

void test_escape_custom_escape_sequence() {
    string input = "abc$";
    string chars = "$";
    string esc = "/";
    string expected = "abc/$";
    string actual = escape(input, chars, esc);
    assert(actual == expected && "Custom escape sequence");
}

void test_escape_already_escaped_in_input() {
    string input = "\\\\$"; // Escaped backslash and $
    string expected = "\\\\\\\\\\$"; // Expect: \\\$
    string actual = escape(input);
    assert(actual == expected && "Already escaped in input");
}

void test_escape_multiple_chars() {
    string input = "$\\\"'`";
    string expected = "\\$\\\\\\\"\\'\\`";
    string actual = escape(input);
    assert(actual == expected && "Multiple chars");
}

TEST(test_escape_empty_input);
TEST(test_escape_no_special_chars);
TEST(test_escape_single_char);
TEST(test_escape_already_escaped);
TEST(test_escape_no_chars);
TEST(test_escape_custom_chars);
TEST(test_escape_mixed_content);
TEST(test_escape_custom_chars2);
TEST(test_escape_custom_escape_sequence);
TEST(test_escape_already_escaped_in_input);
TEST(test_escape_multiple_chars);
#endif
