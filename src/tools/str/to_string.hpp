#pragma once

#include <string>

using namespace std;

namespace tools::str {


    string to_string(bool b, const string& t = "true", const string& f = "false") {
        return b ? t : f;
    }

    template<class T> string to_string(const T& t) {
        stringstream sstr;    
        sstr << t;    
        return sstr.str();
    }
    
}

#ifdef TEST

#include "../utils/Test.h"
#include "../utils/assert.hpp"

using namespace tools::str;
using namespace tools::utils;

void test_tools_str_to_string_bool_basic() {
    string result = to_string(true);
    assert(result == "true" && "Default true string");
    
    result = to_string(false);
    assert(result == "false" && "Default false string");
}

void test_tools_str_to_string_bool_custom_strings() {
    string result = to_string(true, "yes", "no");
    assert(result == "yes" && "Custom true string");
    
    result = to_string(false, "yes", "no");
    assert(result == "no" && "Custom false string");
}

void test_tools_str_to_string_bool_empty_strings() {
    string result = to_string(true, "", "");
    assert(result == "" && "Empty true string");
    
    result = to_string(false, "", "");
    assert(result == "" && "Empty false string");
}

// Register tests
TEST(test_tools_str_to_string_bool_basic);
TEST(test_tools_str_to_string_bool_custom_strings);
TEST(test_tools_str_to_string_bool_empty_strings);

#endif
