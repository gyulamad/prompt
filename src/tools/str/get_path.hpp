#pragma once

#include <string>

#include "fix_path.hpp"

using namespace std;

namespace fs = filesystem;

namespace tools::str {

    string get_path(const string& pname) {
        return fix_path(fs::path(pname).parent_path().string());
    }
    
}

#ifdef TEST

using namespace tools::str;


#endif
