#pragma once

#include <cassert>

#include "../TEST.hpp"
#include "../tools.hpp"

void test_str_get_rest_basic() {
    string orig = "This is a long long .. string that end with this.";

    // Test when index is 0
    assert(str_get_rest(orig, 0) == "This is a long long .. string that end with this.");

    // Test for various indices
    assert(str_get_rest(orig, 1) == "his is a long long .. string that end with this.");
    assert(str_get_rest(orig, 2) == "is is a long long .. string that end with this.");
    assert(str_get_rest(orig, 5) == "is a long long .. string that end with this.");
    assert(str_get_rest(orig, 10) == "long long .. string that end with this.");

    // Test for index equal to string length
    assert(str_get_rest(orig, orig.size()) == "");

    // Test for index greater than string length
    assert(str_get_rest(orig, orig.size() + 1) == "");
}

void test_str_get_rest_edge_cases() {
    string emptyStr = "";

    // Test with an empty string
    assert(str_get_rest(emptyStr, 0) == "");
    assert(str_get_rest(emptyStr, 1) == "");

    string singleChar = "A";

    // Test with a single-character string
    assert(str_get_rest(singleChar, 0) == "A");
    assert(str_get_rest(singleChar, 1) == "");
    assert(str_get_rest(singleChar, 2) == "");
}

void test_tools() {
    TEST(test_str_get_rest_basic);
    TEST(test_str_get_rest_edge_cases);
}