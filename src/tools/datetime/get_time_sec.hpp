#pragma once

#include <string>

#include "default_get_time_ms.hpp"

using namespace std;

namespace tools::datetime {

    // Declaration in the header file (e.g., datetime.hpp)
    extern time_t (*get_time_sec)();
    
    time_t (*get_time_sec)() = default_get_time_sec;
    
}

#ifdef TEST

using namespace tools::datetime;

// Test that get_time_sec returns the correct conversion from milliseconds
void test_get_time_sec_conversion() {
    // Get time in milliseconds and seconds
    time_ms ms = get_time_ms();
    time_sec sec = get_time_sec();
    
    // Calculate expected seconds
    time_sec expected_sec = ms / second_ms;
    
    // Allow for small timing differences during execution
    time_sec diff = sec > expected_sec ? sec - expected_sec : expected_sec - sec;
    assert(diff <= 1 && "get_time_sec should return milliseconds divided by second_ms");
}

// Test that get_time_sec is consistent with get_time_ms
void test_get_time_sec_consistency() {
    time_ms ms = get_time_ms();
    time_sec sec = get_time_sec();
    
    // sec should be approximately ms / second_ms
    assert(sec * second_ms <= ms && "get_time_sec should be consistent with get_time_ms");
    assert((sec + 1) * second_ms >= ms && "get_time_sec should be consistent with get_time_ms");
}

TEST(test_get_time_sec_conversion);
TEST(test_get_time_sec_consistency);
#endif
