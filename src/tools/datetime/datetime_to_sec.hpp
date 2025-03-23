#pragma once

#include <string>

#include "datetime_defs.hpp"
#include "datetime_to_ms.hpp"

using namespace std;

namespace tools::datetime {

    time_t datetime_to_sec(const string& date) {
        return datetime_to_ms(date) / second_ms;
    }
    
}

#ifdef TEST

using namespace tools::datetime;

void test_datetime_to_sec() {
    time_t actual;

    // Full datetime string
    assert(datetime_to_sec("2023-10-05 14:30:45.123") == 1696516245 && "Full datetime test failed");

    // Missing milliseconds
    assert(datetime_to_sec("2023-10-05 14:30:45") == 1696516245 && "Missing milliseconds test failed");

    // Missing seconds
    assert(datetime_to_sec("2023-10-05 14:30") == 1696516200 && "Missing seconds test failed");

    // Missing minutes
    actual = datetime_to_sec("2023-10-05 14");
    assert(actual == 1696514400 && "Missing minutes test failed");

    // Missing hours
    assert(datetime_to_sec("2023-10-05") == 1696464000 && "Missing hours test failed");

    // Missing day
    assert(datetime_to_sec("2023-10") == 1696118400 && "Missing day test failed");

    // Missing month
    actual = datetime_to_sec("2023");
    assert(actual == 1672531200 && "Missing month test failed");

    // Empty string
    bool thrown = false;
    try {
        datetime_to_sec("");
    } catch (...) {
        thrown = true;
    }
    assert(thrown && "Empty string test failed");

    // Invalid input (should default to epoch)
    thrown = false;
    try {
        datetime_to_sec("invalid-date");
    } catch (...) {
        thrown = true;
    }
    assert(thrown && "Invalid input test failed");
}

TEST(test_datetime_to_sec);
#endif
