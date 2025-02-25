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
        for (size_t i = 0; i < trimedLength; i++) 
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