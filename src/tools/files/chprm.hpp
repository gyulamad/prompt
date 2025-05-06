#pragma once

#include <string>
#include <cstring>     // Added for strerror
#include <grp.h>
#include <unistd.h>
#include <sys/stat.h> // For chmod (permissions)
#include "../utils/ERROR.h"
#include "../str/to_string.hpp"

using namespace std;
using namespace tools::utils;
using namespace tools::str;

namespace tools::files {

    void chprm(const string& filename, mode_t mode) {
        // 3. Set the file permissions.
        if (::chmod(filename.c_str(), mode) == -1)
            throw ERROR("Error  changing permissions of " + filename + ": " + to_string(strerror(errno)));
    }

}