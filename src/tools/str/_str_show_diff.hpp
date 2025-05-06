#pragma once

#include <iostream>

#include "../utils/ANSI_FMT.h"

#include "str_diff_t.hpp"

using namespace std;
using namespace tools::utils;

namespace tools::str {

    // Function to display a single diff block
    void str_show_diff(const str_diff_t& diff) {
        cout << "changed line(s) " << diff.bound[0] << ".." << diff.bound[1] << ":\n";
        for (const string& line: diff.added) {
            if (line.empty()) continue;
            cout << ANSI_FMT(ANSI_FMT_C_GREEN, "+ " + line) << "\n";
        }
        for (const string& line: diff.removed) {
            if (line.empty()) continue;
            cout << ANSI_FMT(ANSI_FMT_C_RED, "- " + line) << "\n";
        }
    }
    
}

#ifdef TEST

using namespace tools::str;


#endif
