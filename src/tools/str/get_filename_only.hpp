#pragma once

#include <string>
#include <filesystem>

using namespace std;

namespace fs = filesystem;

namespace tools::str {

    string get_filename_only(const string& fname) {
        return fs::path(fname).stem().string();
    }
    
}

#ifdef TEST

using namespace tools::str;


#endif
