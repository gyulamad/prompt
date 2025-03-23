#pragma once

#include <string>
#include <vector>

#include "str_diff_t.hpp"
#include "str_get_diffs.hpp"
#include "str_show_diff.hpp"

using namespace std;

namespace tools::str {

    // Function to compute and display all differences between two strings
    vector<str_diff_t> str_diffs_show(const string& s1, const string& s2) {
        vector<str_diff_t> diffs = str_get_diffs(s1, s2);
        for (const str_diff_t& diff : diffs) {
            str_show_diff(diff);
        }
        return diffs;
    }
    
}

#ifdef TEST

using namespace tools::str;


#endif
