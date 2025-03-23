#pragma once

#include <string>
#include <filesystem>

#include "fix_path.hpp"

using namespace std;

namespace fs = filesystem;

namespace tools::str {

    string get_absolute_path(const string& fname) {
        return fix_path(fs::absolute(fs::path(fname)).string());
    }
    
}

#ifdef TEST

using namespace tools::str;


#endif
