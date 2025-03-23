#pragma once

#include <string>
#include <vector>

#include "explode.hpp"
#include "implode.hpp"

using namespace std;

namespace tools::str {

    string remove_extension(const string& fname) {
        vector<string> splits = explode(".", fname);
        splits.pop_back(); // remove last element
        return implode(".", splits);
    }

}

#ifdef TEST

using namespace tools::str;


#endif
