#pragma once

#include <string>

using namespace std;

namespace tools::str {

    // PHP like function to convert string to upper case
    string strtoupper(const string& s) {
        string result = s; // Create a copy of the input string
        for (char& c : result) // Iterate over each character
            c = toupper(static_cast<unsigned char>(c)); // Convert to upper case
        return result;
    }
    
}

#ifdef TEST

using namespace tools::str;


void test_strtoupper_basic_conversion() {
    string input = "hello world";
    string expected = "HELLO WORLD";
    string actual = strtoupper(input);
    assert(actual == expected && "test_strtoupper_basic_conversion failed");
}

void test_strtoupper_mixed_case() {
    string input = "HeLLo WoRLd";
    string expected = "HELLO WORLD";
    string actual = strtoupper(input);
    assert(actual == expected && "test_strtoupper_mixed_case failed");
}

void test_strtoupper_already_uppercase() {
    string input = "HELLO WORLD";
    string expected = "HELLO WORLD";
    string actual = strtoupper(input);
    assert(actual == expected && "test_strtoupper_already_uppercase failed");
}

void test_strtoupper_empty_string() {
    string input = "";
    string expected = "";
    string actual = strtoupper(input);
    assert(actual == expected && "test_strtoupper_empty_string failed");
}

void test_strtoupper_with_numbers_and_symbols() {
    string input = "123 abc^!%&#$@";
    string expected = "123 ABC^!%&#$@";
    string actual = strtoupper(input);
    assert(actual == expected && "test_strtoupper_with_numbers_and_symbols failed");
}


TEST(test_strtoupper_basic_conversion);
TEST(test_strtoupper_mixed_case);
TEST(test_strtoupper_already_uppercase);
TEST(test_strtoupper_empty_string);
TEST(test_strtoupper_with_numbers_and_symbols);
#endif
