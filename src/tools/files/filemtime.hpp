#pragma once

#include <string>
#include <chrono>
#include "filemtime_sec.hpp"
#include "../utils/time.hpp"

using namespace std;

namespace tools::files {

    time_t filemtime(const string& filename) {
        return filemtime_sec(filename);
    }

}