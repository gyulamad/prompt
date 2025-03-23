#pragma once

#include <chrono>

using namespace std;
using namespace chrono;

namespace tools::datetime {

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
    
}

#ifdef TEST

using namespace tools::datetime;


#endif
