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

#ifdef TEST

#include "../utils/Test.hpp"

using namespace tools::cmd;

void test_Parameter_constructor() {
    Parameter param = {"test", true, "test help"};
    assert(param.name == "test" && "Name not initialized correctly");
    assert(param.optional == true && "Optional not initialized correctly");
    assert(param.help == "test help" && "Help not initialized correctly");
}

TEST(test_Parameter_constructor);

#endif
