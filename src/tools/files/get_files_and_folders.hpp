#pragma once

#include <vector>
#include <string>
#include <filesystem> // Requires C++17 or later

using namespace std;

namespace fs = filesystem; // Alias for convenience

namespace tools::files {

    vector<string> get_files_and_folders(const string& base_folder, const string& keyword = "") {
        vector<string> result;
        for (const auto& entry : filesystem::recursive_directory_iterator(base_folder)) {
            if (entry.is_directory() || entry.is_regular_file()) {
                string path = entry.path().string();
                if (keyword.empty() || path.find(keyword) != string::npos) {
                    result.push_back(path);
                }
            }
        }
        return result;
    }

}