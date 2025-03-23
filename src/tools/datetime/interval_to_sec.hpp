#pragma once

#include <string>
#include <vector>
#include <unordered_map>

#include "../utils/ERROR.hpp"
#include "../regx/regx_match.hpp"

#include "datetime_defs.hpp"

using namespace std;
using namespace tools::utils;
using namespace tools::regx;

namespace tools::datetime {

    time_t interval_to_sec(const string& interval_str) {
        vector<string> matches;
        if (!regx_match("([0-9]+)([a-zA-Z])", interval_str, &matches) || matches.size() != 3)
            throw ERROR("Invalid interval format: " + interval_str);
        int mul = atoi(matches[1].c_str());
        unordered_map<string, time_t> tmap = {
            { "s", 1 }, { "S", 1 },
            { "m", minute_sec },
            { "h", hour_sec }, { "H", hour_sec },
            { "d", day_sec }, { "D", day_sec },
            { "w", week_sec }, { "W", week_sec },
            { "M", week_sec*4 },
            { "y", day_sec*365 }, { "Y", day_sec*365 },
        };

        auto it = tmap.find(matches[2]);
        if (it == tmap.end())
            throw ERROR("Invalid time unit: " + matches[2]);
        
        return it->second * mul;
    }
    
}

#ifdef TEST

using namespace tools::datetime;

void test_interval_to_sec() {
    // Test case 1: Seconds
    assert(interval_to_sec("10s") == 10 && "10 seconds failed");
    assert(interval_to_sec("5S") == 5 && "5 seconds failed");

    // Test case 2: Minutes
    assert(interval_to_sec("3m") == 3 * minute_sec && "3 minutes failed");
    assert(interval_to_sec("1M") == 4 * week_sec && "1 month failed"); // Approximation

    // Test case 3: Hours
    assert(interval_to_sec("2h") == 2 * hour_sec && "2 hours failed");
    assert(interval_to_sec("4H") == 4 * hour_sec && "4 hours failed");

    // Test case 4: Days
    assert(interval_to_sec("1d") == 1 * day_sec && "1 day failed");
    assert(interval_to_sec("7D") == 7 * day_sec && "7 days failed");

    // Test case 5: Weeks
    assert(interval_to_sec("1w") == 1 * week_sec && "1 week failed");
    assert(interval_to_sec("2W") == 2 * week_sec && "2 weeks failed");

    // Test case 6: Years
    assert(interval_to_sec("1y") == 1 * day_sec * 365 && "1 year failed");
    assert(interval_to_sec("2Y") == 2 * day_sec * 365 && "2 years failed");

    // Test case 7: Invalid interval format
    bool thrown = false;
    try {
        interval_to_sec("abc");
    } catch (const exception& e) {
        thrown = true;
        string errmsg = string(e.what());
        assert(str_contains(errmsg, "Invalid interval format: abc") && "Invalid format exception failed");
    }
    assert(thrown && "Invalid format should throw an exception");

    // Test case 8: Unsupported time unit
    thrown = false;
    try {
        interval_to_sec("5z");
    } catch (const exception& e) {
        thrown = true;
        string errmsg = string(e.what());
        assert(str_contains(errmsg, "Invalid time unit: z") && "Unsupported unit exception failed");
    }
    assert(thrown && "Unsupported unit should throw an exception");
}

TEST(test_interval_to_sec);
#endif
