#pragma once

#include <string>
#include <algorithm>

namespace tools::str {

    inline std::string toUpperFirst(const std::string& str) {
        if (str.empty()) {
            return str;
        }
        std::string result = str;
        result[0] = toupper(result[0]);
        return result;
    }

}
