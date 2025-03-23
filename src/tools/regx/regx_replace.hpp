#pragma once

#include <string>

// NOTE: Do not compiles with -Ofast + -fsanitize=address
//       or use: __attribute__((optimize("O0")))
// https://gcc.gnu.org/bugzilla/show_bug.cgi?id=105562#c27
#include <regex>

#include "../utils/ERROR.hpp"

using namespace std;
using namespace regex_constants;
using namespace tools::utils;

namespace tools::regx {

    /// @brief Replace first regex match in string.
    /// @param pattern regex pattern to match 
    /// @param str input string
    /// @param replace string to replace matches with
    /// @return string with first match replaced
    inline string regx_replace(const string& pattern, const string& str, const string& replace) {
        if (pattern.empty()) throw ERROR("Regex pattern can not be empty"); // Explicitly throw for empty patterns
        regex r(pattern);
        return regex_replace(str, r, replace, regex_constants::format_first_only);
    }
    
}

#ifdef TEST

using namespace tools::regx;

// Test cases for regx_replace
void test_regx_replace_single_match() {
    string input = "hello world";
    string pattern = "world";
    string replace = "universe";
    string expected = "hello universe";
    string actual = regx_replace(pattern, input, replace);
    assert(actual == expected && "test_regx_replace_single_match failed");
}

void test_regx_replace_no_match() {
    string input = "hello world";
    string pattern = "foo";
    string replace = "bar";
    string expected = "hello world";
    string actual = regx_replace(pattern, input, replace);
    assert(actual == expected && "test_regx_replace_no_match failed");
}

void test_regx_replace_first_occurrence_only() {
    string input = "apple apple apple";
    string pattern = "apple";
    string replace = "fruit";
    string expected = "fruit apple apple";
    string actual = regx_replace(pattern, input, replace);
    assert(actual == expected && "test_regx_replace_first_occurrence_only failed");
}

void test_regx_replace_empty_input() {
    string input = "";
    string pattern = "foo";
    string replace = "bar";
    string expected = "";
    string actual = regx_replace(pattern, input, replace);
    assert(actual == expected && "test_regx_replace_empty_input failed");
}

void test_regx_replace_empty_pattern() {
    bool thrown = false;
    try {
        string input = "hello world";
        string pattern = "";
        string replace = "bar";
        regx_replace(pattern, input, replace);
    } catch (...) {
        thrown = true;
    }
    assert(thrown && "test_regx_replace_empty_pattern failed");
}

// Register tests
TEST(test_regx_replace_single_match);
TEST(test_regx_replace_no_match);
TEST(test_regx_replace_first_occurrence_only);
TEST(test_regx_replace_empty_input);
TEST(test_regx_replace_empty_pattern);
#endif
