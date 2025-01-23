#pragma once

#include <string>
#include <fstream>
#include <filesystem> // Requires C++17 or later

using namespace std;

namespace fs = filesystem; // Alias for convenience

namespace tools {
    
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

    string file_get_contents(const string& filename) {
        // Open the file in binary mode and position the cursor at the end
        ifstream file(filename, ios::in | ios::binary);
        if (!file.is_open()) {
            throw ios_base::failure("Failed to open file: " + filename);
        }

        // Seek to the end to determine file size
        file.seekg(0, ios::end);
        streamsize size = file.tellg();
        file.seekg(0, ios::beg);

        // Read file content into a string
        string content(size, '\0');
        if (!file.read(&content[0], size)) {
            throw ios_base::failure("Failed to read file: " + filename);
        }

        return content;
    }

    void file_put_contents(const string& filename, const string& content) {
        ofstream file(filename, ios::out | ios::binary);
        if (!file.is_open()) {
            throw ios_base::failure("Failed to open file: " + filename);
        }

        if (!file.write(content.data(), content.size())) {
            throw ios_base::failure("Failed to write to file: " + filename);
        }
    }

}