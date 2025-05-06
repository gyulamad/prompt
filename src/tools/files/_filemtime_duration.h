#include <chrono>
#include <filesystem>

#include "../utils/ERROR.h"

using namespace std;
using namespace tools::utils;

namespace fs = filesystem;

namespace tools::files {
    chrono::system_clock::duration filemtime_duration(const string& filename);
}