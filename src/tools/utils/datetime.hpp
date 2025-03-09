#pragma once

#include <thread>
#include <iomanip>
#include <unordered_map>

#include "regx.hpp"
#include "strings.hpp"

#define __DATE_TIME__ tools::ms_to_datetime()

using namespace chrono;

namespace tools::utils {

    typedef long long time_ms;
    typedef long long time_sec;

    const time_ms second_ms = 1000;
    const time_ms minute_ms = 60 * second_ms;
    const time_ms hour_ms = 60 * minute_ms;
    const time_ms day_ms = 24 * hour_ms;
    const time_ms week_ms = 7 * day_ms;

    const time_sec minute_sec = 60;
    const time_sec hour_sec = 60 * minute_sec;
    const time_sec day_sec = 24 * hour_sec;
    const time_sec week_sec = 7 * day_sec;



    // ------------------------------------------------------------
    // ------------------------------------------------------------
    // ACTUAL ACCSESS TO CURRENT TIME
    // ------------------------------------------------------------
    // ------------------------------------------------------------

    // Declaration in the header file (e.g., datetime.hpp)
    extern time_t (*get_time_ms)();

    // Definition in the source file (e.g., datetime.cpp)
    time_t default_get_time_ms() {
        // Get the current time point
        auto now = chrono::system_clock::now();

        // Convert to milliseconds since the Unix epoch
        auto millis = chrono::duration_cast<chrono::milliseconds>(
            now.time_since_epoch()
        );

        // Return the numeric value
        return millis.count();
    }


    time_t (*get_time_ms)() = default_get_time_ms;

    // Declaration in the header file (e.g., datetime.hpp)
    extern time_t (*get_time_sec)();

    // Definition in the source file (e.g., datetime.cpp)
    time_t default_get_time_sec() {
        return get_time_ms() / second_ms;
    }

    time_t (*get_time_sec)() = default_get_time_sec;

    
    // ------------------------------------------------------------
    // ------------------------------------------------------------
    // ------------------------------------------------------------
    // ------------------------------------------------------------
    // ------------------------------------------------------------

    
    inline bool is_valid_datetime(const string& datetime) {
        if (datetime.empty()) return false;
    
        const string tpl = "0000-01-01 00:00:00.000";
        size_t len = datetime.length();
        if (len > tpl.length()) return false;
    
        for (size_t i = 0; i < len; ++i) {
            char expected = tpl[i];
            char actual = datetime[i];
    
            if (expected == '0') {
                if (!isdigit(actual)) return false;
            } else if (expected == '-') {
                if (actual != '-') return false;
            } else if (expected == ':') {
                if (actual != ':') return false;
            } else if (expected == '.') {
                if (actual != '.') return false;
            } else if (expected == ' ') {
                if (actual != ' ') return false;
            }
        }
    
        // Check if a '.' exists at position 19 (millisecond separator)
        if (len > 19 && datetime[19] == '.') {
            // If '.' is present, ensure there are exactly 3 digits after it
            if (len != 23 || 
                !isdigit(datetime[20]) || 
                !isdigit(datetime[21]) || 
                !isdigit(datetime[22])) {
                return false;
            }
        }
    
        // Parse components (unchanged)
        int year = 0, month = 1, day = 1, hour = 0, minute = 0, second = 0, millisecond = 0;
        try {
            if (len >= 4) year = stoi(datetime.substr(0, 4));
            if (len >= 7) month = stoi(datetime.substr(5, 2));
            if (len >= 10) day = stoi(datetime.substr(8, 2));
            if (len >= 13) hour = stoi(datetime.substr(11, 2));
            if (len >= 16) minute = stoi(datetime.substr(14, 2));
            if (len >= 19) second = stoi(datetime.substr(17, 2));
            if (len >= 23) millisecond = stoi(datetime.substr(20, 3));
        } catch (...) {
            return false;
        }
    
        // Validate ranges (unchanged)
        if (year < 0 || month < 1 || month > 12 || day < 1 || day > 31 ||
            hour < 0 || hour > 23 || minute < 0 || minute > 59 ||
            second < 0 || second > 59 || millisecond < 0 || millisecond > 999) {
            return false;
        }
    
        // Validate days in months (unchanged)
        if (day > 30 && (month == 4 || month == 6 || month == 9 || month == 11)) return false;
        if (day > 29 && month == 2) return false;
        if (day > 28 && month == 2 && (year % 4 != 0 || (year % 100 == 0 && year % 400 != 0))) return false;
    
        return true;
    }

    inline string normalize_datetime(const string& datetime) {
        string tpl = "0000-01-01 00:00:00.000";
        const string trimed = trim(datetime);
        const size_t trimedLength = trimed.length();

        // Only iterate up to the minimum of the two lengths
        for (size_t i = 0; i < min(trimedLength, tpl.length()); i++)
            tpl[i] = trimed[i];

        return tpl;
    }


    /// @brief 
    /// @deprecated use datetime_to_ms() instead
    /// @param datetime 
    /// @return 
    inline time_t date_parse_ms(const string& date_string) {
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

    /// @brief 
    /// @deprecated use date_to_sec() instead
    /// @param datetime 
    /// @return 
    inline time_t date_parse_sec(const string& date_string) {
        return date_parse_ms(date_string) / (time_t)second_ms;
    }

    inline time_t datetime_to_ms(const string& datetime) {
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

    inline time_t date_to_ms(const string& date) {
        return datetime_to_ms(date);
    }

    inline time_t datetime_to_sec(const string& date) {
        return datetime_to_ms(date) / second_ms;
    }

    inline time_t date_to_sec(const string& date) {
        return datetime_to_ms(date) / second_ms;
    }

    inline string ms_to_datetime(time_t ms = get_time_ms(), const char* fmt = "%Y-%m-%d %H:%M:%S", bool millis = true, bool local = false) {
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

    inline string sec_to_datetime(time_t sec, const char *fmt = "%Y-%m-%d %H:%M:%S", bool local = false) {
        return ms_to_datetime((time_t)sec * second_ms, fmt, false, local);
    }

    inline string ms_to_date(time_t ms = get_time_ms(), const char* fmt = "%Y-%m-%d", bool local = false) {
        return ms_to_datetime(ms, fmt, false, local);
    }

    inline string sec_to_date(time_t sec, const char *fmt = "%Y-%m-%d", bool local = false) {
        return ms_to_datetime((time_t)sec * second_ms, fmt, false, local);
    }

    // // Function to convert timestamp to date string
    // inline string sec_to_date(time_t timestamp) {
    //     tm *tm_ptr = gmtime(&timestamp);
    //     stringstream ss;
    //     ss << put_time(tm_ptr, "%Y-%m-%d");
    //     return ss.str();
    // }

    // // Function to convert date string to timestamp
    // inline time_t date_to_sec(const string &date_str) {
    //     tm tm_ptr = {};
    //     stringstream ss(date_str);
    //     ss >> get_time(&tm_ptr, "%Y-%m-%d");
    //     return mktime(&tm_ptr);
    // }

    inline time_t get_day_first_sec(time_t sec = get_time_sec()) {
        return date_to_sec(sec_to_date(sec));
    }

    // interval

    inline time_t interval_to_sec(const string& interval_str) {
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

#include "Test.hpp"

using namespace tools::utils;

// Test that get_time_ms returns a reasonable current time
void test_get_time_ms_current() {
    // Get current time using the function
    time_ms t1 = get_time_ms();
    
    // Get current time using standard chrono
    auto now = chrono::system_clock::now();
    auto millis = chrono::duration_cast<chrono::milliseconds>(
        now.time_since_epoch()
    ).count();
    
    // The two times should be very close
    // Allow small tolerance of 100ms for execution time difference
    time_ms diff = t1 > millis ? t1 - millis : millis - t1;
    assert(diff < 100 && "get_time_ms should return approximately current time");
}

// Test that consecutive calls to get_time_ms are monotonically increasing
void test_get_time_ms_monotonic() {
    time_ms t1 = get_time_ms();
    this_thread::sleep_for(chrono::milliseconds(10));
    time_ms t2 = get_time_ms();
    
    assert(t2 > t1 && "get_time_ms should be monotonically increasing");
}

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

// Test time elapses correctly
void test_time_elapsed() {
    time_ms start = get_time_ms();
    this_thread::sleep_for(chrono::milliseconds(100));
    time_ms end = get_time_ms();
    
    time_ms elapsed = end - start;
    assert(elapsed >= 100 && "At least 100ms should have elapsed");
    // Allow some overhead for thread scheduling, but not too much
    assert(elapsed < 200 && "Elapsed time should be reasonable");
}

void test_normalize_datetime_basic() {
    string input = "2023-10-05 14:30:45.123";
    string result = normalize_datetime(input);
    string expected = "2023-10-05 14:30:45.123";
    assert(result == expected && "Basic datetime normalization");
}

void test_normalize_datetime_shorter_input() {
    string input = "2023-10-05 14:30";
    string result = normalize_datetime(input);
    string expected = "2023-10-05 14:30:00.000";
    assert(result == expected && "Shorter input (missing seconds and milliseconds)");
}

void test_normalize_datetime_longer_input() {
    string input = "2023-10-05 14:30:45.123456";
    string result = normalize_datetime(input);
    string expected = "2023-10-05 14:30:45.123";
    assert(result == expected && "Longer input (extra characters ignored)");
}

void test_normalize_datetime_empty_input() {
    string input = "";
    string result = normalize_datetime(input);
    string expected = "0000-01-01 00:00:00.000";
    assert(result == expected && "Empty input");
}

void test_normalize_datetime_whitespace_input() {
    string input = "   2023-10-05 14:30:45.123   ";
    string result = normalize_datetime(input);
    string expected = "2023-10-05 14:30:45.123";
    assert(result == expected && "Input with leading/trailing whitespace");
}

void test_normalize_datetime_partial_input() {
    string input = "2023";
    string result = normalize_datetime(input);
    string expected = "2023-01-01 00:00:00.000";
    assert(result == expected && "Partial input (only year)");
}

void test_normalize_datetime_invalid_characters() {
    string input = "2023-10-05 14:30:45.123abc";
    string result = normalize_datetime(input);
    string expected = "2023-10-05 14:30:45.123";
    assert(result == expected && "Invalid characters ignored");
}

void test_is_valid_datetime() {
    // Valid datetimes
    assert(is_valid_datetime("2023") == true); // Year only
    assert(is_valid_datetime("2023-01") == true); // Year and month
    assert(is_valid_datetime("2023-01-01") == true); // Year, month, and day
    assert(is_valid_datetime("2023-01-01 12") == true); // Year, month, day, and hour
    assert(is_valid_datetime("2023-01-01 12:34") == true); // Year, month, day, hour, and minute
    assert(is_valid_datetime("2023-01-01 12:34:45") == true); // Year, month, day, hour, minute, and second
    assert(is_valid_datetime("2023-01-01 12:34:45.123") == true); // Full datetime

    // Invalid datetimes
    assert(is_valid_datetime("") == false); // Empty string
    assert(is_valid_datetime("2023-13") == false); // Invalid month
    assert(is_valid_datetime("2023-01-32") == false); // Invalid day
    assert(is_valid_datetime("2023-02-29") == false); // Non-leap year
    assert(is_valid_datetime("2023-01-01 24") == false); // Invalid hour
    assert(is_valid_datetime("2023-01-01 12:60") == false); // Invalid minute
    assert(is_valid_datetime("2023-01-01 12:34:60") == false); // Invalid second
    assert(is_valid_datetime("2023-01-01 12:34:45.") == false); // Incomplete milliseconds

    // Invalid format
    assert(is_valid_datetime("2023/01/01") == false); // Wrong separator
    assert(is_valid_datetime("2023-01-01T12:34:45") == false); // Wrong separator
    assert(is_valid_datetime("2023-01-01 12-34-45") == false); // Wrong separator
}

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

void test_date_to_ms() {
    // Full date string
    assert(date_to_ms("2023-10-05") == 1696464000000LL && "Full date test failed");

    // Missing day
    assert(date_to_ms("2023-10") == 1696118400000LL && "Missing day test failed");

    // Missing month
    time_t actual = date_to_ms("2023");
    assert(actual == 1672531200000LL && "Missing month test failed");


    // Empty string
    bool thrown = false;
    try {
        date_to_ms("");
    } catch (...) {
        thrown = true;
    }
    assert(thrown && "Empty string test failed");

    // Invalid input (should default to epoch)
    thrown = false;
    try {
        date_to_ms("invalid-date");
    } catch (...) {
        thrown = true;
    }
    assert(thrown && "Invalid input test failed");
}

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

// Mock get_time_ms for testing purposes
time_t mock_get_time_ms() {
    return 1696516245123LL; // Example timestamp: "2023-10-05 14:30:45.123"
}

void test_ms_to_datetime() {
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

void test_sec_to_datetime() {
    // Default format (UTC)
    assert(sec_to_datetime(1696516245) == "2023-10-05 14:30:45" && "Default format failed");

    // Custom format (UTC)
    assert(sec_to_datetime(1696516245, "%Y/%m/%d %H:%M:%S") == "2023/10/05 14:30:45" && "Custom format failed");

    // Local time (structure check)
    string local_result = sec_to_datetime(1696516245, "%Y-%m-%d %H:%M:%S", true);
    assert(local_result.size() == 19 && "Local time format failed");
}

void test_ms_to_date() {
    // Default format (UTC)
    assert(ms_to_date(1696516245123LL) == "2023-10-05" && "Default format failed");

    // Custom format (UTC)
    assert(ms_to_date(1696516245123LL, "%Y/%m/%d") == "2023/10/05" && "Custom format failed");

    // Local time (structure check)
    string local_result = ms_to_date(1696516245123LL, "%Y-%m-%d", true);
    assert(local_result.size() == 10 && "Local time format failed");
}

void test_sec_to_date() {
    // Default format (UTC)
    assert(sec_to_date(1696516245) == "2023-10-05" && "Default format failed");

    // Custom format (UTC)
    assert(sec_to_date(1696516245, "%Y/%m/%d") == "2023/10/05" && "Custom format failed");

    // Local time (structure check)
    string local_result = sec_to_date(1696516245, "%Y-%m-%d", true);
    assert(local_result.size() == 10 && "Local time format failed");
}

// Mock get_time_sec for testing purposes
time_t mock_get_time_sec() {
    return 1696516245; // Example timestamp: "2023-10-05 14:30:45"
}

void test_get_day_first_sec() {
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

// Register all tests
TEST(test_get_time_ms_current);
TEST(test_get_time_ms_monotonic);
TEST(test_get_time_sec_conversion);
TEST(test_get_time_sec_consistency);
TEST(test_time_elapsed);
TEST(test_normalize_datetime_basic);
TEST(test_normalize_datetime_shorter_input);
TEST(test_normalize_datetime_longer_input);
TEST(test_normalize_datetime_empty_input);
TEST(test_normalize_datetime_whitespace_input);
TEST(test_normalize_datetime_partial_input);
TEST(test_normalize_datetime_invalid_characters);
TEST(test_is_valid_datetime);
TEST(test_date_parse_ms_basic);
TEST(test_date_parse_ms_partial_input);
TEST(test_date_parse_ms_empty_input);
TEST(test_date_parse_ms_invalid_characters);
TEST(test_date_parse_ms_missing_milliseconds);
TEST(test_date_parse_sec_basic);
TEST(test_date_parse_sec_partial_input);
TEST(test_date_parse_sec_empty_input);
TEST(test_date_parse_sec_invalid_characters);
TEST(test_date_parse_sec_missing_milliseconds);
TEST(test_datetime_to_ms);
TEST(test_date_to_ms);
TEST(test_datetime_to_sec);
TEST(test_date_to_sec);
TEST(test_ms_to_datetime);
TEST(test_sec_to_datetime);
TEST(test_ms_to_date);
TEST(test_sec_to_date);
TEST(test_get_day_first_sec);
TEST(test_interval_to_sec);
#endif