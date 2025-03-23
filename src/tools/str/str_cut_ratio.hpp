#pragma once

#include <string>

#include "../utils/ERROR.hpp"

using namespace std;
using namespace tools::utils;

namespace tools::str {

    /**
     * Splits a string into two parts based on a given ratio.
     * @param str The input string.
     * @param ratio The ratio at which to split the string (default is 0.5).
     * @return A pair of strings representing the two parts.
     */
    pair<string, string> str_cut_ratio(const string& str, double ratio = 0.5) {
        // Ensure the ratio is between 0 and 1
        if (ratio < 0.0 || ratio > 1.0)
            throw ERROR("Ratio must be between 0.0 and 1.0");

        size_t splitPoint = static_cast<size_t>(str.length() * ratio);

        // Split the string into two parts
        string firstPart = str.substr(0, splitPoint);
        string secondPart = str.substr(splitPoint);

        return {firstPart, secondPart};
    }

}

#ifdef TEST

using namespace tools::str;

// Test cases for str_cut_ratio
void test_str_cut_ratio_when_ratio_is_default() {
    string input = "abcdefgh";
    pair<string, string> expected = {"abcd", "efgh"};
    pair<string, string> actual = str_cut_ratio(input);
    assert(actual.first == expected.first && actual.second == expected.second &&
           "test_str_cut_ratio_when_ratio_is_default failed");
}

void test_str_cut_ratio_when_ratio_is_half() {
    string input = "abcdefgh";
    pair<string, string> expected = {"abcd", "efgh"};
    pair<string, string> actual = str_cut_ratio(input, 0.5);
    assert(actual.first == expected.first && actual.second == expected.second &&
           "test_str_cut_ratio_when_ratio_is_half failed");
}

void test_str_cut_ratio_when_ratio_is_zero() {
    string input = "abcdefgh";
    pair<string, string> expected = {"", "abcdefgh"};
    pair<string, string> actual = str_cut_ratio(input, 0.0);
    assert(actual.first == expected.first && actual.second == expected.second &&
           "test_str_cut_ratio_when_ratio_is_zero failed");
}

void test_str_cut_ratio_when_ratio_is_one() {
    string input = "abcdefgh";
    pair<string, string> expected = {"abcdefgh", ""};
    pair<string, string> actual = str_cut_ratio(input, 1.0);
    assert(actual.first == expected.first && actual.second == expected.second &&
           "test_str_cut_ratio_when_ratio_is_one failed");
}

void test_str_cut_ratio_when_ratio_is_small() {
    string input = "abcdefgh";
    pair<string, string> expected = {"a", "bcdefgh"};
    pair<string, string> actual = str_cut_ratio(input, 0.125);
    assert(actual.first == expected.first && actual.second == expected.second &&
           "test_str_cut_ratio_when_ratio_is_small failed");
}

void test_str_cut_ratio_when_ratio_is_large() {
    string input = "abcdefgh";
    pair<string, string> expected = {"abcdefg", "h"};
    pair<string, string> actual = str_cut_ratio(input, 0.875);
    assert(actual.first == expected.first && actual.second == expected.second &&
           "test_str_cut_ratio_when_ratio_is_large failed");
}

void test_str_cut_ratio_when_input_is_empty() {
    string input = "";
    pair<string, string> expected = {"", ""};
    pair<string, string> actual = str_cut_ratio(input, 0.5);
    assert(actual.first == expected.first && actual.second == expected.second &&
           "test_str_cut_ratio_when_input_is_empty failed");
}

void test_str_cut_ratio_when_ratio_is_invalid() {
    bool thrown = false;
    try {
        str_cut_ratio("abcdefgh", 1.5);
    } catch (const exception& e) {
        thrown = true;
    }
    assert(thrown && "test_str_cut_ratio_when_ratio_is_invalid failed");
}


TEST(test_str_cut_ratio_when_ratio_is_default);
TEST(test_str_cut_ratio_when_ratio_is_half);
TEST(test_str_cut_ratio_when_ratio_is_zero);
TEST(test_str_cut_ratio_when_ratio_is_one);
TEST(test_str_cut_ratio_when_ratio_is_small);
TEST(test_str_cut_ratio_when_ratio_is_large);
TEST(test_str_cut_ratio_when_input_is_empty);
TEST(test_str_cut_ratio_when_ratio_is_invalid);
#endif
