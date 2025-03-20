#pragma once

#include <string>
#include <filesystem>

#include "strings.hpp"

using namespace std;
using namespace tools::utils;

namespace fs = filesystem;

namespace tools::utils {

    string fix_path(const string& pname) {
        // Convert to filesystem path and normalize it
        fs::path path = fs::path(pname).lexically_normal();
        
        // Convert to string and ensure forward slashes
        string result = path.string();
        replace(result.begin(), result.end(), '\\', '/');
        
        // If the path is empty or just "/", return it as is
        if (result.empty() || result == "/") return result;
    
        // Remove trailing slash if present (unless it's the root)
        if (result.length() > 1 && result.back() == '/') result.pop_back();
    
        return result;
    }

    string get_absolute_path(const string& fname) {
        return fix_path(fs::absolute(fs::path(fname)).string());
    }

    string get_path(const string& pname) {
        return fix_path(fs::path(pname).parent_path().string());
    }

    string get_filename_only(const string& fname) {
        return fs::path(fname).stem().string();
    }

    string remove_extension(const string& fname) {
        vector<string> splits = explode(".", fname);
        splits.pop_back(); // remove last element
        return implode(".", splits);
    }

    string remove_path(const string& fname) {
        vector<string> splits = explode("/", fname);
        return splits.back(); // Return the last element
    }

    string replace_extension(const string& fname, const string& newext) {
        return remove_extension(fname) + (!newext.empty() ? str_starts_with(newext, ".") ? newext : ("." + newext) : "");
    }
}