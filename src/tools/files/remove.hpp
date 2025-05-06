#pragma once

#include <string>
#include <iostream>
#include <filesystem> // Requires C++17 or later
#include <sys/stat.h> // For chmod (permissions)
#include "../utils/ERROR.h"
#include "../str/str_cut_begin.hpp"

using namespace std;
using namespace tools::utils;
using namespace tools::str;

namespace fs = filesystem; // Alias for convenience

namespace tools::files {

    bool remove(const string& path, bool throws = true) {
        bool ok = fs::remove(path);
        if (!ok && throws) throw ERROR("Unable to remove: " + str_cut_begin(path));
        return ok;
    }

}