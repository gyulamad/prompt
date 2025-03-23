#pragma once

#include <string>

#include "datetime_defs.hpp"
#include "date_parse_ms.hpp"

using namespace std;

namespace tools::datetime {

    /// @brief 
    /// @deprecated use date_to_sec() instead
    /// @param datetime 
    /// @return 
    time_t date_parse_sec(const string& date_string) {
        return date_parse_ms(date_string) / (time_t)second_ms;
    }
    
}

#ifdef TEST

using namespace tools::datetime;

void test_date_parse_sec_basic() {
    string date_string = "2023-10-05 14:30:45.123";
    time_t result = date_parse_sec(date_string);
    time_t expected = 1696516245; // 2023-10-05 14:30:45
    assert(result == expected && "Basic datetime parsing (seconds)");
}

void test_date_parse_sec_partial_input() {
    string date_string = "2023-10-05";
    time_t result = date_parse_sec(date_string);
    time_t expected = 1696464000; // 2023-10-05 00:00:00
    assert(result == expected && "Partial input (date only)");
}

void test_date_parse_sec_empty_input() {
    string date_string = "";
    time_t result = date_parse_sec(date_string);
    time_t expected = 0;
    assert(result == expected && "Empty input");
}

void test_date_parse_sec_invalid_characters() {
    string date_string = "2023-10-05 14:30:45.123abc";
    time_t result = date_parse_sec(date_string);
    time_t expected = 1696516245; // Invalid characters ignored
    assert(result == expected && "Invalid characters ignored");
}

void test_date_parse_sec_missing_milliseconds() {
    string date_string = "2023-10-05 14:30:45";
    time_t result = date_parse_sec(date_string);
    time_t expected = 1696516245; // Milliseconds ignored
    assert(result == expected && "Missing milliseconds");
}

TEST(test_date_parse_sec_basic);
TEST(test_date_parse_sec_partial_input);
TEST(test_date_parse_sec_empty_input);
TEST(test_date_parse_sec_invalid_characters);
TEST(test_date_parse_sec_missing_milliseconds);
#endif
