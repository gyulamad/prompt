#pragma once

#include <string>

#include "datetime_defs.hpp"
#include "get_time_ms.hpp"

using namespace std;

namespace tools::datetime {

    // Definition in the source file (e.g., datetime.cpp)
    time_t default_get_time_sec() {
        return get_time_ms() / second_ms;
    }
    
}

#ifdef TEST

using namespace tools::datetime;


#endif
