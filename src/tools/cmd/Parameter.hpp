#pragma once

#include <string>

using namespace std;

namespace tools::cmd {

    struct Parameter {
        string name;
        bool optional;
        string help;
    };

}
