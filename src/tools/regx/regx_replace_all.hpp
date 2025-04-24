#pragma once

#include <string>

// NOTE: Do not compiles with -Ofast + -fsanitize=address
//       or use: __attribute__((optimize("O0")))
//       or: #pragma GCC optimize("O0")
// https://gcc.gnu.org/bugzilla/show_bug.cgi?id=105562#c27
#include <regex>

#include "../utils/ERROR.hpp"

using namespace std;
using namespace tools::utils;

#pragma GCC optimize("O0")
namespace tools::regx {

    /// @brief Replace all regex matches in string.
    /// @param pattern regex pattern to match
    /// @param str input string 
    /// @param replace string to replace matches with 
    /// @return string with all matches replaced
    __attribute__((optimize("O0")))
    inline string regx_replace_all(const string& pattern, const string& str, const string& replace) {
        if (pattern.empty()) throw ERROR("Regex pattern can not be empty"); // Explicitly throw for empty patterns
        regex r(pattern);
        return regex_replace(str, r, replace);
    }
    
}
#pragma GCC reset_options

#ifdef TEST

using namespace tools::regx;

void test_regx_replace_all() {
    string input = "apple apple apple";
    string pattern = "apple";
    string replace = "fruit";
    string expected = "fruit fruit fruit";
    string actual = regx_replace_all(pattern, input, replace);
    assert(actual == expected && "test_regx_replace_all failed");
}

// Test cases for regx_replace_all
void test_regx_replace_all_multiple_matches() {
    string input = "apple banana apple";
    string pattern = "apple";
    string replace = "fruit";
    string expected = "fruit banana fruit";
    string actual = regx_replace_all(pattern, input, replace);
    assert(actual == expected && "test_regx_replace_all_multiple_matches failed");
}

void test_regx_replace_all_no_match() {
    string input = "hello world";
    string pattern = "foo";
    string replace = "bar";
    string expected = "hello world";
    string actual = regx_replace_all(pattern, input, replace);
    assert(actual == expected && "test_regx_replace_all_no_match failed");
}

void test_regx_replace_all_empty_input() {
    string input = "";
    string pattern = "foo";
    string replace = "bar";
    string expected = "";
    string actual = regx_replace_all(pattern, input, replace);
    assert(actual == expected && "test_regx_replace_all_empty_input failed");
}

void test_regx_replace_all_empty_pattern() {
    bool thrown = false;
    try {
        string input = "hello world";
        string pattern = "";
        string replace = "bar";
        regx_replace_all(pattern, input, replace);
    } catch (...) {
        thrown = true;
    }
    assert(thrown && "test_regx_replace_all_empty_pattern failed");
}

void test_regx_replace_all_regex_special_characters() {
    string input = "123-456-7890";
    string pattern = "\\d{3}-\\d{3}-\\d{4}";
    string replace = "[phone]";
    string expected = "[phone]";
    string actual = regx_replace_all(pattern, input, replace);
    assert(actual == expected && "test_regx_replace_all_regex_special_characters failed");
}

TEST(test_regx_replace_all);
TEST(test_regx_replace_all_multiple_matches);
TEST(test_regx_replace_all_no_match);
TEST(test_regx_replace_all_empty_input);
TEST(test_regx_replace_all_empty_pattern);
TEST(test_regx_replace_all_regex_special_characters);
#endif
