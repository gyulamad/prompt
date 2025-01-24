#pragma once

// #include <iostream>
#include <string>
#include <filesystem> 
#include <unistd.h>
#include <limits.h>

using namespace std;
namespace fs = filesystem;

namespace tools {

    // Function to extract the directory path using filesystem
    string get_path(const string& filepath) {
        fs::path path(filepath);
        return path.parent_path().string(); // Get the directory portion
    }

    string get_exec_path() {
        char path[PATH_MAX];
        ssize_t count = readlink("/proc/self/exe", path, PATH_MAX);
        if (count != -1) {
            path[count] = '\0';
            return get_path(string(path));
        }
        return "";  
    }

}