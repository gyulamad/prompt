#pragma once

#include <string>
#include <vector>

#include "explode.hpp"

using namespace std;

namespace tools::str {

    string remove_path(const string& fname) {
        vector<string> splits = explode("/", fname);
        return splits.back(); // Return the last element
    }
    
}

#ifdef TEST

using namespace tools::str;


#endif
