#pragma once

#include <string>
#include <algorithm>

namespace tools::str {

    inline std::string toLower(const std::string& str) {
        std::string result = str;
        std::transform(result.begin(), result.end(), result.begin(), ::tolower);
        return result;
    }

}
