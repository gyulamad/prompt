#pragma once

#include <string>

using namespace std;

namespace tools::str {

    bool str_ends_with(const string& str, const string& suffix) {
        // Check if the suffix is longer than the string
        if (str.size() < suffix.size()) return false;

        // Compare the end of the string with the suffix
        return str.compare(str.size() - suffix.size(), suffix.size(), suffix) == 0;
    }

}

#ifdef TEST

using namespace tools::str;

void test_str_ends_with_basic() {
    string str = "hello world";
    string suffix = "world";
    bool actual = str_ends_with(str, suffix);
    bool expected = true;
    assert(actual == expected && "Basic ends_with");
}

void test_str_ends_with_empty_suffix() {
    string str = "hello world";
    string suffix = "";
    bool actual = str_ends_with(str, suffix);
    bool expected = true;
    assert(actual == expected && "Empty suffix");
}

void test_str_ends_with_empty_string() {
    string str = "";
    string suffix = "world";
    bool actual = str_ends_with(str, suffix);
    bool expected = false;
    assert(actual == expected && "Empty string");
}

void test_str_ends_with_suffix_longer_than_string() {
    string str = "world";
    string suffix = "hello world";
    bool actual = str_ends_with(str, suffix);
    bool expected = false;
    assert(actual == expected && "Suffix longer than string");
}

void test_str_ends_with_no_match() {
    string str = "hello world";
    string suffix = "hello";
    bool actual = str_ends_with(str, suffix);
    bool expected = false;
    assert(actual == expected && "No match");
}

void test_str_ends_with_case_sensitive() {
    string str = "Hello World";
    string suffix = "world";
    bool actual = str_ends_with(str, suffix);
    bool expected = false;
    assert(actual == expected && "Case sensitive");
}


TEST(test_str_ends_with_basic);
TEST(test_str_ends_with_empty_suffix);
TEST(test_str_ends_with_empty_string);
TEST(test_str_ends_with_suffix_longer_than_string);
TEST(test_str_ends_with_no_match);
TEST(test_str_ends_with_case_sensitive);
#endif
