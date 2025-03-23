#pragma once

#include <string>

#include "str_replace.hpp"
#include "escape.hpp"

using namespace std;

namespace tools::str {

    string json_escape(const string& s) {
        return str_replace("\n", "\\n", escape(s, "\\\""));
    }
    
}

#ifdef TEST

using namespace tools::str;


#endif
