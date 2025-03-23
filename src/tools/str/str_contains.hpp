#pragma once

#include <string>

using namespace std;

namespace tools::str {

    bool str_contains(const string& str, const string& substring) {
        // Use string::find to check if the substring exists
        return str.find(substring) != string::npos;
    }
    
}

#ifdef TEST

using namespace tools::str;


#endif
