#pragma once

#include <string>
#include <chrono>
#include "filemtime_duration.hpp"
#include "../utils/time.hpp"

using namespace std;

namespace tools::files {

    sec_t filemtime_sec(const string& filename) {
        return chrono::duration_cast<chrono::seconds>(filemtime_duration(filename)).count();
    }

}