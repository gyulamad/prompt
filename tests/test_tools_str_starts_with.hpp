#pragma once

#include <cassert>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <string>
#include <iostream>

#include "../TEST.hpp"
#include "../tools.hpp"

void test_tools_str_starts_with_all_in_one() {
    // Test: Prefix matches the start of the string
    assert(str_starts_with("hello", "he"));
    assert(str_starts_with("hello", "hello"));

    // Test: Prefix does not match
    assert(!str_starts_with("hello", "world"));
    assert(!str_starts_with("hello", "ell"));
    assert(!str_starts_with("hello", "lo"));

    // Test: Prefix longer than string
    assert(!str_starts_with("hi", "hello"));

    // Test: Empty prefix
    assert(str_starts_with("hello", ""));
    assert(str_starts_with("", ""));

    // Test: Empty string with non-empty prefix
    assert(!str_starts_with("", "hello"));

    // Test: Case sensitivity
    assert(!str_starts_with("Hello", "he"));
    assert(str_starts_with("Hello", "He"));

    // Test: Prefix with special characters
    assert(str_starts_with("hello!", "hello"));
    assert(!str_starts_with("hello!", "hello@"));
}


void test_tools_str_starts_with() {
  TEST(test_tools_str_starts_with_all_in_one);
}