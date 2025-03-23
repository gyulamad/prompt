#pragma once

namespace tools::datetime {

    #define __DATE_TIME__ datetime::ms_to_datetime()

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
    
}

#ifdef TEST

using namespace tools::datetime;


#endif
