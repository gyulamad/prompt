#pragma once

#include <string>
#include <chrono>
#include "filemtime_duration.hpp"
#include "../utils/time.hpp"

using namespace std;

namespace tools::files {

    ms_t filemtime_ms(const string& filename) {
        return chrono::duration_cast<chrono::milliseconds>(filemtime_duration(filename)).count();
    }

}