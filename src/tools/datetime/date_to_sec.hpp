#pragma once

#include <string>

#include "datetime_defs.hpp"
#include "datetime_to_ms.hpp"

using namespace std;

namespace tools::datetime {

    time_t date_to_sec(const string& date) {
        return datetime_to_ms(date) / second_ms;
    }
    
}

#ifdef TEST

using namespace tools::datetime;

void test_date_to_sec() {
    // Full date string
    assert(date_to_sec("2023-10-05") == 1696464000 && "Full date test failed");

    // Missing day
    assert(date_to_sec("2023-10") == 1696118400 && "Missing day test failed");

    // Missing month
    time_t actual = date_to_sec("2023");
    assert(actual == 1672531200 && "Missing month test failed");

    // Empty string
    bool thrown = false;
    try {
        date_to_sec("");
    } catch (...) {
        thrown = true;
    }
    assert(thrown && "Empty string test failed");

    // Invalid input (should default to epoch)
    thrown = false;
    try {
        date_to_sec("invalid-date");
    } catch (...) {
        thrown = true;
    }
    assert(thrown && "Invalid input test failed");
}

TEST(test_date_to_sec);
#endif
