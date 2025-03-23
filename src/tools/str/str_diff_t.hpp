#pragma once

#include <string>
#include <vector>

using namespace std;

namespace tools::str {

    // Struct to represent a single diff block
    typedef struct {
        int bound[2];          // Line range where the difference occurs [start, end]
        vector<string> added;  // Lines added in s2
        vector<string> removed; // Lines removed from s1
    } str_diff_t;
    
}

#ifdef TEST

using namespace tools::str;


#endif
