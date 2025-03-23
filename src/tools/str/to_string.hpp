#pragma once

#include <string>

using namespace std;

namespace tools::str {


    string to_string(bool b, const string& t = "true", const string& f = "false") { // TODO to common libs
        return b ? t : f;
    }

    template<class T> string to_string(const T& t) {
        stringstream sstr;    
        sstr << t;    
        return sstr.str();
    }
    
}

#ifdef TEST

using namespace tools::str;


#endif
