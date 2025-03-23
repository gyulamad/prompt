#pragma once

#include <string>
#include <algorithm>
#include <filesystem>

using namespace std;

namespace fs = filesystem;

namespace tools::str {

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
    
}

#ifdef TEST

using namespace tools::str;


#endif
