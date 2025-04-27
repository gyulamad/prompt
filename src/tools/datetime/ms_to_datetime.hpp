#pragma once

#include <string>

#include "datetime_defs.hpp"
#include "get_time_ms.hpp"

using namespace std;

namespace tools::datetime {

    string ms_to_datetime(time_t ms = get_time_ms(), const char* fmt = "%Y-%m-%d %H:%M:%S", bool millis = true, bool local = false) {
        long sec = (signed)(ms / second_ms);
        long mil = (signed)(ms % second_ms);

        struct tm converted_time;
        if (local) localtime_r(&sec, &converted_time);
        else gmtime_r(&sec, &converted_time);

        ostringstream oss;
        oss << put_time(&converted_time, fmt);

        if (millis) oss << "." << setfill('0') << setw(3) << mil;

        return oss.str();
    }
    
}

#ifdef TEST

using namespace tools::datetime;

void test_ms_to_datetime() {
    // LCOV_EXCL_START
    // Mock get_time_ms for testing purposes
    auto mock_get_time_ms = []() -> time_t {
        return 1696516245123LL; // Example timestamp: "2023-10-05 14:30:45.123"
    };
    // LCOV_EXCL_STOP

    // Mock current time
    auto original_get_time_ms = get_time_ms;
    get_time_ms = mock_get_time_ms;

    // Default format with milliseconds (UTC)
    assert(ms_to_datetime(1696516245123LL) == "2023-10-05 14:30:45.123" && "Default format with milliseconds failed");

    // Default format without milliseconds (UTC)
    assert(ms_to_datetime(1696516245123LL, "%Y-%m-%d %H:%M:%S", false) == "2023-10-05 14:30:45" && "Default format without milliseconds failed");

    // Custom format with milliseconds (UTC)
    assert(ms_to_datetime(1696516245123LL, "%Y/%m/%d %H:%M:%S", true) == "2023/10/05 14:30:45.123" && "Custom format with milliseconds failed");

    // Custom format without milliseconds (UTC)
    assert(ms_to_datetime(1696516245123LL, "%Y/%m/%d %H:%M:%S", false) == "2023/10/05 14:30:45" && "Custom format without milliseconds failed");

    // Local time (this depends on the system's timezone, so we only check structure)
    string local_result = ms_to_datetime(1696516245123LL, "%Y-%m-%d %H:%M:%S", true, true);
    assert(local_result.size() == 23 && "Local time format failed");

    // Restore original get_time_ms
    get_time_ms = original_get_time_ms;
}

TEST(test_ms_to_datetime);
#endif
