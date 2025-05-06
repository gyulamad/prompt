#include <chrono>
#include <filesystem>

#include "../utils/ERROR.h"
#include "filemtime_duration.h"

using namespace std;
using namespace tools::utils;

namespace fs = filesystem;

namespace tools::files {
    chrono::system_clock::duration filemtime_duration(const string& filename) {
        if (filename.empty()) ERROR("Filename can not be empty");
        fs::path file_path(filename);
        if (!fs::exists(file_path)) {
            throw ERROR("File does not exist: " + filename);
        }
        fs::file_time_type file_time = fs::last_write_time(file_path);
        chrono::system_clock::time_point system_time = chrono::clock_cast<chrono::system_clock>(file_time);
        return system_time.time_since_epoch();
    }
}