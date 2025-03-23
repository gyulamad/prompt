#pragma once

#include <string>
#include <time.h>

#include "datetime_defs.hpp"

using namespace std;

namespace tools::datetime {

    /// @brief 
    /// @deprecated use datetime_to_ms() instead
    /// @param datetime 
    /// @return 
    time_t date_parse_ms(const string& date_string) {
        if (date_string.empty()) return 0;

        struct tm time_info = {};
        int milliseconds = 0;
        
        size_t size = date_string.size();
        time_info.tm_year = (size > 3 ? stoi(date_string.substr(0, 4)) : 1970) - 1900;
        time_info.tm_mon = (size > 6 ? stoi(date_string.substr(5, 2)) : 1) - 1;
        time_info.tm_mday = size > 9 ? stoi(date_string.substr(8, 2)) : 1;
        time_info.tm_hour = size > 12 ? stoi(date_string.substr(11, 2)) : 0;
        time_info.tm_min = size > 15 ? stoi(date_string.substr(14, 2)) : 0;
        time_info.tm_sec = size > 18 ? stoi(date_string.substr(17, 2)) : 0;
        milliseconds = size > 22 ? stoi(date_string.substr(20, 3)) : 0;

        // Convert the struct tm to milliseconds
        time_t seconds = mktime(&time_info);
        return seconds * (time_t)second_ms + milliseconds;
    }
    
}

#ifdef TEST

using namespace tools::datetime;

void test_date_parse_ms_basic() {
    string date_string = "2023-10-05 14:30:45.123";
    time_t result = date_parse_ms(date_string);
    time_t expected = 1696516245123; // Calculated manually or using a reliable library
    assert(result == expected && "Basic datetime parsing (milliseconds)");
}

void test_date_parse_ms_partial_input() {
    string date_string = "2023-10-05";
    time_t result = date_parse_ms(date_string);
    time_t expected = 1696464000000; // 2023-10-05 00:00:00.000
    assert(result == expected && "Partial input (date only)");
}

void test_date_parse_ms_empty_input() {
    string date_string = "";
    time_t result = date_parse_ms(date_string);
    time_t expected = 0;
    assert(result == expected && "Empty input");
}

void test_date_parse_ms_invalid_characters() {
    string date_string = "2023-10-05 14:30:45.123abc";
    time_t result = date_parse_ms(date_string);
    time_t expected = 1696516245123; // Invalid characters ignored
    assert(result == expected && "Invalid characters ignored");
}

void test_date_parse_ms_missing_milliseconds() {
    string date_string = "2023-10-05 14:30:45";
    time_t result = date_parse_ms(date_string);
    time_t expected = 1696516245000; // Milliseconds default to 0
    assert(result == expected && "Missing milliseconds");
}

TEST(test_date_parse_ms_basic);
TEST(test_date_parse_ms_partial_input);
TEST(test_date_parse_ms_empty_input);
TEST(test_date_parse_ms_invalid_characters);
TEST(test_date_parse_ms_missing_milliseconds);
#endif
