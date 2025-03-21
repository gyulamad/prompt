#pragma once

#include <string>
#include <vector>

#include "../../utils/files.hpp"

using namespace std;
using namespace tools::utils;

// Helper to set up and clean up filesystem for tests
namespace test_fs {

    void setup_dirs(const vector<string>& dirs) {
        for (const string& dir : dirs) {
            if (!mkdir(dir, true)) {
                throw runtime_error("Failed to create test directory: " + dir);
            }
        }
    }

    void create_file(const string& path, const string& content = "") {
        file_put_contents(path, content, false, true);
    }

    void set_file_time(const string& path, ms_t ms) {
        auto sys_time = chrono::system_clock::time_point(chrono::milliseconds(ms));
        auto file_time = chrono::clock_cast<chrono::file_clock>(sys_time);
        fs::last_write_time(path, file_time);
    }

    void cleanup(const string& base_dir) {
        if (fs::exists(base_dir)) {
            try {
                fs::remove_all(base_dir);
            } catch (const fs::filesystem_error& e) {
                cerr << "Cleanup failed: " << e.what() << endl;
            }
        }
    }
}