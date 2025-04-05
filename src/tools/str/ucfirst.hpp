#pragma once

#include <string>
#include <algorithm>

using namespace std;

namespace tools::str {

    inline string ucfirst(const string& str) {
        if (str.empty()) {
            return str;
        }
        string result = str;
        result[0] = toupper(result[0]);
        return result;
    }

}
