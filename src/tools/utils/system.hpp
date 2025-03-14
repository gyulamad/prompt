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

#include "ERROR.hpp"
#include "io.hpp"

using namespace std;
namespace fs = filesystem;

namespace tools::utils {

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

    bool is_process_running(const string& processName, long& pid) {
        DIR *dir;
        struct dirent *ent;
        if ((dir = opendir("/proc")) != NULL) {
            while ((ent = readdir(dir)) != NULL) {
                if (ent->d_type == DT_DIR) {
                    char* endptr;
                    pid = strtol(ent->d_name, &endptr, 10);
                    if (*endptr == '\0') {
                        ifstream cmdline_file("/proc/" + string(ent->d_name) + "/cmdline");
                        if (cmdline_file.is_open()) {
                            string cmdline;
                            getline(cmdline_file, cmdline);

                            size_t pos = cmdline.find(processName);
                            if (pos != string::npos) {
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
            cerr << "Error reading /proc: " << strerror(errno) << endl;
            return false;
        }
        return false;
    }
    bool is_process_running(const string& processName) {
        long pids;
        return is_process_running(processName, pids);
    }

    void sleep_ms(long ms) {
        this_thread::sleep_for(chrono::milliseconds(ms));
    }
    
    static string execute(const char* cmd) {
        string result = "";
        char buffer[128];
        FILE* pipe = popen(cmd, "r");
        if (!pipe) throw ERROR("popen() failed!");
        while (!feof(pipe))
            if (fgets(buffer, 128, pipe) != nullptr) result += buffer;
        int status = pclose(pipe);
        if (status == -1) throw ERROR("pclose() failed!");
        return result;
    }

    size_t get_threads_count() {
        ifstream stat("/proc/self/stat");
        if (!stat) return 0;
        string line;
        getline(stat, line);
        istringstream iss(line);
        string token;
        for (int i = 1; i <= 20; ++i) {
            iss >> token;
            if (i == 20) return stoul(token);
        }
        return 0;
    }
}

#ifdef TEST

#include "../utils/Test.hpp"

using namespace tools::utils;

void test_execute_successful_output() {
    string result = execute("echo Hello");
    // On some systems, echo appends a newline
    assert((result == "Hello\n" || result == "Hello") && "execute should capture echo output");
}

void test_execute_empty_output() {
    string result = execute("true"); // POSIX command that does nothing
    assert(result.empty() && "execute should return empty string for no output");
}

void test_execute_multiline_output() {
    // Use a portable command; 'ls -d .' on Unix, 'dir' on Windows might need adjustment
    string result = execute("printf 'Line1\nLine2\n'");
    assert(result == "Line1\nLine2\n" && "execute should concatenate multi-line output");
}

// Register tests
TEST(test_execute_successful_output);
TEST(test_execute_empty_output);
TEST(test_execute_multiline_output);
#endif