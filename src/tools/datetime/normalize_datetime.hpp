#pragma once

#include <string>

#include "../str/trim.hpp"

using namespace std;
using namespace tools::str;

namespace tools::datetime {

    string normalize_datetime(const string& datetime) {
        string tpl = "0000-01-01 00:00:00.000";
        const string trimed = trim(datetime);
        const size_t trimedLength = trimed.length();

        // Only iterate up to the minimum of the two lengths
        for (size_t i = 0; i < min(trimedLength, tpl.length()); i++)
            tpl[i] = trimed[i];

        return tpl;
    }
    
}

#ifdef TEST

using namespace tools::datetime;

void test_normalize_datetime_basic() {
    string input = "2023-10-05 14:30:45.123";
    string result = normalize_datetime(input);
    string expected = "2023-10-05 14:30:45.123";
    assert(result == expected && "Basic datetime normalization");
}

void test_normalize_datetime_shorter_input() {
    string input = "2023-10-05 14:30";
    string result = normalize_datetime(input);
    string expected = "2023-10-05 14:30:00.000";
    assert(result == expected && "Shorter input (missing seconds and milliseconds)");
}

void test_normalize_datetime_longer_input() {
    string input = "2023-10-05 14:30:45.123456";
    string result = normalize_datetime(input);
    string expected = "2023-10-05 14:30:45.123";
    assert(result == expected && "Longer input (extra characters ignored)");
}

void test_normalize_datetime_empty_input() {
    string input = "";
    string result = normalize_datetime(input);
    string expected = "0000-01-01 00:00:00.000";
    assert(result == expected && "Empty input");
}

void test_normalize_datetime_whitespace_input() {
    string input = "   2023-10-05 14:30:45.123   ";
    string result = normalize_datetime(input);
    string expected = "2023-10-05 14:30:45.123";
    assert(result == expected && "Input with leading/trailing whitespace");
}

void test_normalize_datetime_partial_input() {
    string input = "2023";
    string result = normalize_datetime(input);
    string expected = "2023-01-01 00:00:00.000";
    assert(result == expected && "Partial input (only year)");
}

void test_normalize_datetime_invalid_characters() {
    string input = "2023-10-05 14:30:45.123abc";
    string result = normalize_datetime(input);
    string expected = "2023-10-05 14:30:45.123";
    assert(result == expected && "Invalid characters ignored");
}

TEST(test_normalize_datetime_basic);
TEST(test_normalize_datetime_shorter_input);
TEST(test_normalize_datetime_longer_input);
TEST(test_normalize_datetime_empty_input);
TEST(test_normalize_datetime_whitespace_input);
TEST(test_normalize_datetime_partial_input);
TEST(test_normalize_datetime_invalid_characters);
#endif
