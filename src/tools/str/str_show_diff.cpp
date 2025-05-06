#include "str_show_diff.h"
#include "../utils/ANSI_FMT.h"
#include <iostream>

using namespace std;
using namespace tools::utils;

namespace tools::str {

    void str_show_diff(const str_diff_t& diff) {
        cout << "changed line(s) " << diff.bound[0] << ".." << diff.bound[1] << ":\n";
        for (const string& line : diff.added) {
            if (line.empty()) continue;
            cout << ansi_fmt(ANSI_FMT_C_GREEN, "+ " + line) << "\n";
        }
        for (const string& line : diff.removed) {
            if (line.empty()) continue;
            cout << ansi_fmt(ANSI_FMT_C_RED, "- " + line) << "\n";
        }
    }

} // namespace tools::str