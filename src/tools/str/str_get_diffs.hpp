#pragma once

#include <string>
#include <vector>

#include "explode.hpp"
#include "str_diff_t.hpp"

using namespace std;

namespace tools::str {

    // Function to compute differences between two strings
    vector<str_diff_t> str_get_diffs(const string& s1, const string& s2) {
        vector<string> lines1 = explode("\n", s1);
        vector<string> lines2 = explode("\n", s2);

        vector<str_diff_t> diffs;
        size_t i = 0, j = 0;

        while (i < lines1.size() || j < lines2.size()) {
            // Find the start of a difference
            if (i < lines1.size() && j < lines2.size() && lines1[i] == lines2[j]) {
                i++;
                j++;
                continue;
            }

            // Start of a diff block
            int start = i + 1; // 1-based line numbering
            vector<string> added;
            vector<string> removed;

            // Collect removed lines
            while (i < lines1.size() && (j >= lines2.size() || lines1[i] != lines2[j])) {
                removed.push_back(lines1[i]);
                i++;
            }

            // Collect added lines
            while (j < lines2.size() && (i >= lines1.size() || lines1[i] != lines2[j])) {
                added.push_back(lines2[j]);
                j++;
            }

            // Record the diff block
            str_diff_t diff;
            diff.bound[0] = start;
            diff.bound[1] = i; // End of the removed block (1-based)
            diff.added = added;
            diff.removed = removed;
            diffs.push_back(diff);
        }

        return diffs;
    }
    
}

#ifdef TEST

using namespace tools::str;


#endif
