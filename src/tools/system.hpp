#pragma once

// #include <iostream>
#include <string>
#include <filesystem> 
#include <chrono>
#include <iostream>
#include <fstream>
#include <sstream>
#include <cstring>
#include <cerrno>
#include <vector>
#include <thread>

#include <unistd.h>
#include <limits.h>
#include <dirent.h>
#include <sys/types.h>

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

    // // Function to get the current time in milliseconds
    // long long get_time_ms() {
    //     // Get the current time point
    //     auto now = chrono::system_clock::now();

    //     // Convert to milliseconds since the Unix epoch
    //     auto millis = chrono::duration_cast<chrono::milliseconds>(
    //         now.time_since_epoch()
    //     );

    //     // Return the numeric value
    //     return millis.count();
    // }

    bool is_process_running(const std::string& processName) {
        DIR *dir;
        struct dirent *ent;
        if ((dir = opendir("/proc")) != NULL) {
            while ((ent = readdir(dir)) != NULL) {
                if (ent->d_type == DT_DIR) {
                    char* endptr;
                    long pid = strtol(ent->d_name, &endptr, 10);
                    if (*endptr == '\0') {
                        std::ifstream cmdline_file("/proc/" + std::string(ent->d_name) + "/cmdline");
                        if (cmdline_file.is_open()) {
                            std::string cmdline;
                            std::getline(cmdline_file, cmdline);

                            size_t pos = cmdline.find(processName);
                            if (pos != std::string::npos) {
                                cmdline_file.close();
                                closedir(dir);
                                return true;
                            }

                            cmdline_file.close();
                        }
                    }
                }
            }
            closedir(dir);
        } else {
            std::cerr << "Error reading /proc: " << strerror(errno) << std::endl;
            return false;
        }
        return false;
    }

    void sleep_ms(long ms) {
        this_thread::sleep_for(chrono::milliseconds(ms));
    }

}