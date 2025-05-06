#pragma once

#include <iostream>
#include <string>
#include <filesystem> // Requires C++17 or later
#include <sys/stat.h> // For chmod (permissions)

using namespace std;

namespace fs = filesystem; // Alias for convenience

namespace tools::files {

    // Function to create a directory with PHP-like parameters
    [[nodiscard]]
    bool mkdir(const string& directoryPath, int permissions = 0777, bool recursive = false) {
        try {
            bool created = false;

            if (recursive) {
                // Create nested directories
                created = fs::create_directories(directoryPath);
            } else {
                // Create a single directory
                created = fs::create_directory(directoryPath);
            }

            if (created) {
                // Set permissions (only on Unix-like systems)
                #ifdef __unix__
                if (chmod(directoryPath.c_str(), permissions) != 0) {
                    cerr << "Warning: Failed to set permissions for " << directoryPath << endl;
                }
                #endif

                // Directory created
                return true;
            } else if (fs::exists(directoryPath)) {
                // Directory already exists
                return true;
            } else {
                // Failed to create directory
                return false;
            }
        } catch (const fs::filesystem_error& e) {
            // Error
            return false;
        }
    }
    [[nodiscard]]
    bool mkdir(const string& dir, bool recursive, int permission = 0777) {
        return mkdir(dir, permission, recursive);
    }

}