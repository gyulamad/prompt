#pragma once

#include <string>

#include "../utils/ERROR.hpp"

#include "datetime_defs.hpp"
#include "is_valid_datetime.hpp"

using namespace std;
using namespace tools::utils;

namespace tools::datetime {

    time_t datetime_to_ms(const string& datetime) {
        if (datetime.empty() || !is_valid_datetime(datetime)) 
            throw ERROR("Invalid datetime format: " + (datetime.empty() ? "<empty>" : datetime));

        struct tm time_info = {};
        int milliseconds = 0;
        
        size_t size = datetime.size();
        time_info.tm_year = (size > 3 ? stoi(datetime.substr(0, 4)) : 1970) - 1900;
        time_info.tm_mon = (size > 6 ? stoi(datetime.substr(5, 2)) : 1) - 1;
        time_info.tm_mday = size > 9 ? stoi(datetime.substr(8, 2)) : 1;
        time_info.tm_hour = size > 12 ? stoi(datetime.substr(11, 2)) : 0;
        time_info.tm_min = size > 15 ? stoi(datetime.substr(14, 2)) : 0;
        time_info.tm_sec = size > 18 ? stoi(datetime.substr(17, 2)) : 0;
        milliseconds = size > 22 ? stoi(datetime.substr(20, 3)) : 0;

        // Convert the struct tm to milliseconds
        time_t seconds = mktime(&time_info);
        return seconds * second_ms + (unsigned)milliseconds;
    }
    
}

#ifdef TEST

using namespace tools::datetime;

void test_datetime_to_ms() {
    time_t actual;

    // Full datetime string
    assert(datetime_to_ms("2023-10-05 14:30:45.123") == 1696516245123LL && "Full datetime test failed");

    // Missing milliseconds
    assert(datetime_to_ms("2023-10-05 14:30:45") == 1696516245000LL && "Missing milliseconds test failed");

    // Missing seconds
    assert(datetime_to_ms("2023-10-05 14:30") == 1696516200000LL && "Missing seconds test failed");

    // Missing minutes
    actual = datetime_to_ms("2023-10-05 14");
    assert(actual == 1696514400000LL && "Missing minutes test failed");

    // Missing hours
    assert(datetime_to_ms("2023-10-05") == 1696464000000LL && "Missing hours test failed");

    // Missing day
    assert(datetime_to_ms("2023-10") == 1696118400000LL && "Missing day test failed");

    // Missing month
    actual = datetime_to_ms("2023");
    assert(actual == 1672531200000LL && "Missing month test failed");

    // Empty string
    bool thrown = false;
    try {
        datetime_to_ms("");
    } catch (...) {
        thrown = true;
    }
    assert(thrown && "Empty string test failed");

    // Invalid input (should default to epoch)
    thrown = false;
    try {
        datetime_to_ms("invalid-date");
    } catch (...) {
        thrown = true;
    }
    assert(thrown && "Invalid input test failed");
}

TEST(test_datetime_to_ms);
#endif
