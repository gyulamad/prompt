#pragma once

#include <string>
#include <unistd.h>

using namespace std;

namespace tools::files {

    bool unlink(const string& filename) {
        return ::unlink(filename.c_str());
    }

}