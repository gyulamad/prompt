#pragma once

#include <string>
#include <iostream>
#include <filesystem> // Requires C++17 or later
#include "../utils/ERROR.h"

using namespace std;
using namespace tools::utils;

namespace fs = filesystem; // Alias for convenience

namespace tools::files {

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