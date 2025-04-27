#pragma once

#include <string>
#include <vector>
#include <algorithm>

#include "../utils/ERROR.hpp"
#include "../str/str_cut_end.hpp"
#include "../containers/in_array.hpp"

using namespace std;
using namespace tools::utils;
using namespace tools::str;
using namespace tools::containers;

namespace tools::str {

    template <typename T>
    T parse(const string& str) {
        if constexpr (is_same_v<T, string>) {
            return str; // Return the string directly
        } else {
            static_assert(is_arithmetic<T>::value, "T must be an arithmetic type");
            stringstream ss(str);
            T num;
            if (ss >> num) return num;
            throw ERROR("Invalid input string (not a number): " + (str.empty() ? "<empty>" : str_cut_end(str)));
        }
    }

    // Specialization for bool
    template <>
    bool parse<bool>(const string& str) {
        string lower = str;
        transform(lower.begin(), lower.end(), lower.begin(), ::tolower);        
        if (in_array(lower, vector<string>({ "true", "on", "1", "yes"}))) return true;
        if (in_array(lower, vector<string>({ "false", "off", "0", "no"}))) return false;
        throw ERROR("Invalid input string (not a boolean): " + (str.empty() ? "<empty>" : str_cut_end(str)));
    }
    
}

#ifdef TEST

using namespace tools::str;

void test_parse_valid_integer() {
    int expected = 42;
    int actual = parse<int>("42");
    assert(actual == expected && "test_parse_valid_integer failed");
}

void test_parse_valid_double() {
    double expected = 3.14;
    double actual = parse<double>("3.14");
    assert(actual == expected && "test_parse_valid_double failed");
}

void test_parse_negative_number() {
    int expected = -10;
    int actual = parse<int>("-10");
    assert(actual == expected && "test_parse_negative_number failed");
}

void test_parse_invalid_input() {
    bool thrown = false;
    try {
        parse<int>("abc");
    } catch (const exception&) {
        thrown = true;
    }
    assert(thrown && "test_parse_invalid_input failed");
}

void test_parse_empty_string() {
    bool thrown = false;
    try {
        parse<double>("");
    } catch (const exception&) {
        thrown = true;
    }
    assert(thrown && "test_parse_empty_string failed");
}

void test_parse_whitespace_only() {
    bool thrown = false;
    try {
        parse<int>("   ");
    } catch (const exception&) {
        thrown = true;
    }
    assert(thrown && "test_parse_whitespace_only failed");
}

void test_parse_trailing_characters() {    
    int actual = parse<int>("42abc");
    assert(actual == 42 && "test_parse_trailing_characters failed");
}

void test_parse_trailing_characters_negative() {    
    int actual = parse<int>("-42abc");
    assert(actual == -42 && "test_parse_trailing_characters_negative failed");
}

void test_parse_floating_point_with_exponent() {
    double expected = 1.23e3;
    double actual = parse<double>("1.23e3");
    assert(actual == expected && "test_parse_floating_point_with_exponent failed");
}

void test_parse_zero() {
    int expected = 0;
    int actual = parse<int>("0");
    assert(actual == expected && "test_parse_zero failed");
}

void test_parse_large_number() {
    long long expected = 9223372036854775807LL;
    long long actual = parse<long long>("9223372036854775807");
    assert(actual == expected && "test_parse_large_number failed");
}

void test_parse_invalid_boolean() {
    bool thrown = false;
    string expectedError = "Invalid input string (not a boolean): ";
    
    try {
        // Try to parse an invalid boolean string
        parse<bool>("maybe");
    } catch (const exception& e) {
        thrown = true;
        string what = e.what();
        // Check if the error message contains the expected text
        assert(str_contains(what, expectedError) && "Error message doesn't contain expected text");
        // Check if the error message contains the input string
        assert(str_contains(what, "maybe") && "Error message doesn't contain input string");
    }
    
    assert(thrown && "test_parse_invalid_boolean failed - no exception was thrown");
}


TEST(test_parse_valid_integer);
TEST(test_parse_valid_double);
TEST(test_parse_negative_number);
TEST(test_parse_invalid_input);
TEST(test_parse_empty_string);
TEST(test_parse_whitespace_only);
TEST(test_parse_trailing_characters);
TEST(test_parse_trailing_characters_negative);
TEST(test_parse_floating_point_with_exponent);
TEST(test_parse_zero);
TEST(test_parse_large_number);
TEST(test_parse_invalid_boolean);

#endif
