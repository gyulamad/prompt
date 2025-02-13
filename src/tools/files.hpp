#pragma once

#include <iostream>
#include <string>
#include <fstream>
#include <filesystem> // Requires C++17 or later
#include <sys/stat.h> // For chmod (permissions)

#include "ERROR.hpp"
#include "strings.hpp"

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

    bool file_put_contents(const string& filename, const string& content, bool append = false, bool throws = false) {
        // Open the file in the appropriate mode
        ios_base::openmode mode = ios::out | ios::binary;
        if (append) {
            mode |= ios::app; // Append to the file if it exists
        }

        ofstream file(filename, mode);
        if (!file.is_open()) {
            if (throws) throw ios_base::failure("Failed to open file: " + filename);
            else return false;
        }

        // Write the content to the file
        file.write(content.data(), content.size());

        // Check if the write operation failed
        if (file.fail()) {
            file.close(); // Close the file before throwing the exception
            if (throws) throw ios_base::failure("Failed to write to file: " + filename);
            else return false;
        }

        // Flush the stream to ensure the data is written to the file
        file.flush();

        // Close the file
        file.close();

        return true;
    }

    // Function to check if a directory exists
    bool is_dir(const string& path) {
        return fs::exists(path) && fs::is_directory(path);
    }

    // Function to create a directory with PHP-like parameters
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

    bool remove(const string& path, bool throws = true) {
        bool ok = fs::remove(path);
        if (!ok && throws) throw ERROR("Unable to remove: " + str_cut_begin(path));
        return ok;
    }

    bool rename(const string& from, const string& to, bool throws = false) {
        error_code err;
        fs::rename(from, to, err);
        if (err) {
            string errmsg = "File rename failed: " + from + " to " + to + " (" + err.message() + ")";
            if (throws) throw ERROR(errmsg);
            else {
                cerr << errmsg << endl;
                return false;
            }
        }
        return true;
    }

}