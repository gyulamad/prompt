#pragma once

#include <string>

using namespace std;

namespace tools::str {

    // PHP like function to convert string to lower case
    string strtolower(const string& s) {
        string result = s; // Create a copy of the input string
        for (char& c : result) // Iterate over each character
            c = tolower(static_cast<unsigned char>(c)); // Convert to lower case
        return result;        
    }
    
}

#ifdef TEST

using namespace tools::str;


void test_strtolower_basic_conversion() {
    string input = "HELLO WORLD";
    string expected = "hello world";
    string actual = strtolower(input);
    assert(actual == expected && "test_strtolower_basic_conversion failed");
}

void test_strtolower_mixed_case() {
    string input = "HeLLo WoRLd";
    string expected = "hello world";
    string actual = strtolower(input);
    assert(actual == expected && "test_strtolower_mixed_case failed");
}

void test_strtolower_already_lowercase() {
    string input = "hello world";
    string expected = "hello world";
    string actual = strtolower(input);
    assert(actual == expected && "test_strtolower_already_lowercase failed");
}

void test_strtolower_empty_string() {
    string input = "";
    string expected = "";
    string actual = strtolower(input);
    assert(actual == expected && "test_strtolower_empty_string failed");
}

void test_strtolower_with_numbers_and_symbols() {
    string input = "123 ABC^!%&#$@";
    string expected = "123 abc^!%&#$@";
    string actual = strtolower(input);
    assert(actual == expected && "test_strtolower_with_numbers_and_symbols failed");
}

TEST(test_strtolower_basic_conversion);
TEST(test_strtolower_mixed_case);
TEST(test_strtolower_already_lowercase);
TEST(test_strtolower_empty_string);
TEST(test_strtolower_with_numbers_and_symbols);
#endif
