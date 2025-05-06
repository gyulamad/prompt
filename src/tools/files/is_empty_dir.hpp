#pragma once

#include <string>
#include "is_dir.hpp"
#include "../utils/ERROR.h"

using namespace std;
using namespace tools::utils;

namespace tools::files {

    // Function to check if a directory is empty
    bool is_empty_dir(const string& dirpath) {
        if (!is_dir(dirpath)) {
            throw ERROR("Path is not a directory: " + dirpath);
        }

        for (const auto& entry : fs::directory_iterator(dirpath)) {
            (void)entry; // prevent unused variable warning
            return false;
        }

        return true; // Directory is empty
    }

}