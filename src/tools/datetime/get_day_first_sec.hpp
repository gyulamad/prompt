#pragma once

#include <string>

#include "get_time_sec.hpp"
#include "date_to_sec.hpp"
#include "sec_to_date.hpp"

using namespace std;

namespace tools::datetime {

    time_t get_day_first_sec(time_t sec = get_time_sec()) {
        return date_to_sec(sec_to_date(sec));
    }
    
}

#ifdef TEST

using namespace tools::datetime;

void test_get_day_first_sec() {
    // Mock get_time_sec for testing purposes
    time_t mock_get_time_sec() {
        return 1696516245; // Example timestamp: "2023-10-05 14:30:45"
    }

    // Mock current time
    auto original_get_time_sec = get_time_sec;
    get_time_sec = mock_get_time_sec;

    // Test case 1: Default behavior (current time)
    time_t result1 = get_day_first_sec();
    assert(result1 == 1696464000 && "Default behavior failed"); // Midnight of "2023-10-05"

    // Test case 2: Explicit timestamp
    time_t input2 = 1696516245; // "2023-10-05 14:30:45"
    time_t result2 = get_day_first_sec(input2);
    assert(result2 == 1696464000 && "Explicit timestamp failed"); // Midnight of "2023-10-05"

    // Test case 3: Timestamp at midnight
    time_t input3 = 1696464000; // "2023-10-05 00:00:00"
    time_t result3 = get_day_first_sec(input3);
    assert(result3 == 1696464000 && "Midnight timestamp failed");

    // Test case 4: Timestamp just before midnight
    time_t input4 = 1696550399; // "2023-10-05 23:59:59"
    time_t result4 = get_day_first_sec(input4);
    assert(result4 == 1696464000 && "Just before midnight failed");

    // Test case 5: Timestamp on a different day
    time_t input5 = 1696600800; // "2023-10-06 12:00:00"
    time_t result5 = get_day_first_sec(input5);
    assert(result5 == 1696550400 && "Different day failed"); // Midnight of "2023-10-06"

    // Restore original get_time_sec
    get_time_sec = original_get_time_sec;
}

TEST(test_get_day_first_sec);
#endif
