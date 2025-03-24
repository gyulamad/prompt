#pragma once

#include <iostream>
#include <string>
#include <cstring>     // Added for strerror
#include <fstream>
#include <filesystem> // Requires C++17 or later
#include <stdexcept>
#include <chrono>
#include <unistd.h>
#include <pwd.h>
#include <grp.h>
#include <errno.h>
#include <sys/stat.h> // For chmod (permissions)
#include <sys/types.h>

#include "../str/str_cut_begin.hpp"
#include "../str/to_string.hpp"
#include "ERROR.hpp"
#include "time.hpp"

using namespace std;
using namespace tools::str;

namespace fs = filesystem; // Alias for convenience

namespace tools::utils {
    
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
            throw ERROR("Failed to open file: " + filename);
        }

        // Seek to the end to determine file size
        file.seekg(0, ios::end);
        streamsize size = file.tellg();
        file.seekg(0, ios::beg);

        // Read file content into a string
        string content(size, '\0');
        if (!file.read(&content[0], size)) {
            throw ERROR("Failed to read file: " + filename);
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
            if (throws) throw ERROR("Failed to open file: " + filename);
            else return false;
        }

        // Write the content to the file
        file.write(content.data(), content.size());

        // Check if the write operation failed
        if (file.fail()) {
            file.close(); // Close the file before throwing the exception
            if (throws) throw ERROR("Failed to write to file: " + filename);
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

    // Function to change the owner of a file
    void chown(const string& filename, const string& new_owner_username) {
        // 1. Get the UID of the new owner.
        struct passwd *pwd = getpwnam(new_owner_username.c_str());
        if (pwd == nullptr)
            throw ERROR("Error: Could not find user " + new_owner_username);
            
        uid_t new_owner_uid = pwd->pw_uid;

        // 2. Change the owner of the file
        if (::chown(filename.c_str(), new_owner_uid, -1) == -1)  // -1 means don't change the group
            throw ERROR("Error changing owner of " + filename + ": " + to_string(strerror(errno)));
    }

    void chgrp(const string& filename, const string& groupname) {
        // 1. Get the GID of the shared group.
        struct group *grp = getgrnam(groupname.c_str());
        if (grp == nullptr)
            throw ERROR("Error: Could not find group " + groupname);
            
        gid_t shared_gid = grp->gr_gid;
    
        // 2. Change the group ownership of the file.
        if (::chown(filename.c_str(), -1, shared_gid) == -1) // -1 means don't change the user
            throw ERROR("Error changing group of " + filename + ": " + to_string(strerror(errno)));
    }

    void chprm(const string& filename, mode_t mode) {
        // 3. Set the file permissions.
        if (::chmod(filename.c_str(), mode) == -1)
            throw ERROR("Error  changing permissions of " + filename + ": " + to_string(strerror(errno)));
    }

    chrono::system_clock::duration filemtime_duration(const string& filename) {
        if (filename.empty()) ERROR("Filename can not be empty");
        // Convert the filename to a path object
        fs::path file_path(filename);
    
        // Check if the file exists
        if (!fs::exists(file_path)) {
            throw ERROR("File does not exist: " + filename);
        }
    
        // Get the last modification time as file_time_type
        fs::file_time_type file_time = fs::last_write_time(file_path);
    
        // Convert the file_time to a system clock time point
        chrono::system_clock::time_point system_time = chrono::clock_cast<chrono::system_clock>(file_time);
    
        // Convert the system_time to milliseconds since the epoch
        return system_time.time_since_epoch();
    }

    ms_t filemtime_ms(const string& filename) {
        return chrono::duration_cast<chrono::milliseconds>(filemtime_duration(filename)).count();
    }

    sec_t filemtime_sec(const string& filename) {
        return chrono::duration_cast<chrono::seconds>(filemtime_duration(filename)).count();
    }

    time_t filemtime(const string& filename) {
        return filemtime_sec(filename);
    }

    bool unlink(const string& filename) {
        return ::unlink(filename.c_str());
    }
}