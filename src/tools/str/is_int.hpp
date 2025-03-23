#pragma once

#include <string>

#include "is_integer.hpp"

using namespace std;

namespace tools::str {

    bool is_int(const string& s) {
        return is_integer(s);
    }
    
}

#ifdef TEST

using namespace tools::str;

void test_is_int_alias() {
    // Ensure is_int behaves identically to is_integer
    assert(is_int("123") && "test_is_int_alias failed");
    assert(!is_int("12.34") && "test_is_int_alias failed");
    assert(!is_int("") && "test_is_int_alias failed");
}

TEST(test_is_int_alias);
#endif
