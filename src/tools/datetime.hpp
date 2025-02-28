#pragma once

#include <thread>
#include <iomanip>
#include <unordered_map>

#include "regx.hpp"
#include "strings.hpp"

#define __DATE_TIME__ tools::ms_to_datetime()

using namespace chrono;

namespace tools {

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


    // Function to get the current time in milliseconds
    time_ms get_time_ms() {
        // Get the current time point
        auto now = chrono::system_clock::now();

        // Convert to milliseconds since the Unix epoch
        auto millis = chrono::duration_cast<chrono::milliseconds>(
            now.time_since_epoch()
        );

        // Return the numeric value
        return millis.count();
    }

    inline time_sec get_time_sec() {
        return get_time_ms() / second_ms;
    }

    
    // ------------------------------------------------------------
    // ------------------------------------------------------------
    // ------------------------------------------------------------
    // ------------------------------------------------------------
    // ------------------------------------------------------------

    


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
        if (datetime.empty()) return 0;

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

using namespace tools;

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
#endif