#pragma once

#include <string>

#include "datetime_defs.hpp"
#include "ms_to_datetime.hpp"

using namespace std;

namespace tools::datetime {

    string sec_to_date(time_t sec, const char *fmt = "%Y-%m-%d", bool local = false) {
        return ms_to_datetime((time_t)sec * second_ms, fmt, false, local);
    }
    
}

#ifdef TEST

using namespace tools::datetime;

void test_sec_to_date() {
    // Default format (UTC)
    assert(sec_to_date(1696516245) == "2023-10-05" && "Default format failed");

    // Custom format (UTC)
    assert(sec_to_date(1696516245, "%Y/%m/%d") == "2023/10/05" && "Custom format failed");

    // Local time (structure check)
    string local_result = sec_to_date(1696516245, "%Y-%m-%d", true);
    assert(local_result.size() == 10 && "Local time format failed");
}

TEST(test_sec_to_date);
#endif
