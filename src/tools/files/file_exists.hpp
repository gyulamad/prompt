#pragma once

#include <filesystem> // Requires C++17 or later

using namespace std;

namespace fs = filesystem; // Alias for convenience

namespace tools::files {

    /**
     * Mimics PHP's file_exists function.
     * Checks if a file or directory exists at the given path.
     * 
     * @param path The path to the file or directory.
     * @return bool True if the file or directory exists, false otherwise.
     */
    bool file_exists(const string& path) {
        return fs::exists(path);
    }

}