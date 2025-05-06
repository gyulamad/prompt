#pragma once

#include <chrono>
#include <filesystem> // Requires C++17 or later
#include "../utils/ERROR.h"

using namespace std;
using namespace tools::utils;

namespace fs = filesystem; // Alias for convenience

namespace tools::files {

    chrono::system_clock::duration filemtime_duration(const string& filename) {
        if (filename.empty()) ERROR("Filename can not be empty");
        // Convert the filename to a path object
        fs::path file_path(filename);
    
        // Check if the file exists
        if (!fs::exists(file_path)) {
            throw ERROR("File does not exist: " + filename);
        }
    
        // Get the last modification time as file_time_type
        fs::file_time_type file_time = fs::last_write_time(file_path);
    
        // Convert the file_time to a system clock time point
        chrono::system_clock::time_point system_time = chrono::clock_cast<chrono::system_clock>(file_time);
    
        // Convert the system_time to milliseconds since the epoch
        return system_time.time_since_epoch();
    }

}