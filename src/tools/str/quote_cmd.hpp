#pragma once

#include <string>

#include "escape.hpp"

using namespace std;

namespace tools::str {

    string quote_cmd(const string& input) {
        return "\"" + escape(input, "$\\\"") + "\"";
    }
    
}

#ifdef TEST

using namespace tools::str;

void test_quote_cmd_basic() {
    string input = "hello";
    string expected = "\"hello\"";
    string actual = quote_cmd(input);
    assert(actual == expected && "Basic quoting");
}

void test_quote_cmd_with_special_chars() {
    string input = "hello$world\\\"";
    string expected = "\"hello\\$world\\\\\\\"\"";
    string actual = quote_cmd(input);
    assert(actual == expected && "Special characters");
}

void test_quote_cmd_empty_input() {
    string input = "";
    string expected = "\"\"";
    string actual = quote_cmd(input);
    assert(actual == expected && "Empty input");
}

void test_quote_cmd_already_escaped() {
    string input = "hello\\$world";
    string expected = "\"hello\\\\\\$world\"";
    string actual = quote_cmd(input);
    assert(actual == expected && "Already escaped characters");
}

void test_quote_cmd_mixed_content() {
    string input = "hello\"world$";
    string expected = "\"hello\\\"world\\$\"";
    string actual = quote_cmd(input);
    assert(actual == expected && "Mixed content");
}

void test_quote_cmd_only_special_chars() {
    string input = "$\\\"";
    string expected = "\"\\$\\\\\\\"\"";
    string actual = quote_cmd(input);
    assert(actual == expected && "Only special characters");
}

TEST(test_quote_cmd_basic);
TEST(test_quote_cmd_with_special_chars);
TEST(test_quote_cmd_empty_input);
TEST(test_quote_cmd_already_escaped);
TEST(test_quote_cmd_mixed_content);
TEST(test_quote_cmd_only_special_chars);
#endif
