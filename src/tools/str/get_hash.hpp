#pragma once

#include <string>

using namespace std;

namespace tools::str {

    string get_hash(const string& str) {
        return to_string(hash<string>{}(str));
    }
    
}

#ifdef TEST

using namespace tools::str;


#endif
