#pragma once

#include <string>
#include <vector>
#include <sstream>

using namespace std;

namespace tools::str {

    vector<string> split(const string& s) {
        vector<string> parts;
        stringstream ss(s);
        string part;
        while (ss >> part) parts.push_back(part);
        return parts;
    }
    
}

#ifdef TEST

#include "../containers/vector_equal.hpp"

using namespace tools::str;
using namespace tools::containers;

void test_split_single_word() {
    vector<string> expected = {"hello"};
    vector<string> actual = split("hello");
    assert(vector_equal(actual, expected) && "test_split_single_word failed");
}

void test_split_multiple_words() {
    vector<string> expected = {"hello", "world"};
    vector<string> actual = split("hello world");
    assert(vector_equal(actual, expected) && "test_split_multiple_words failed");
}

void test_split_with_extra_whitespace() {
    vector<string> expected = {"hello", "world"};
    vector<string> actual = split("   hello   world   ");
    assert(vector_equal(actual, expected) && "test_split_with_extra_whitespace failed");
}

void test_split_empty_string() {
    vector<string> expected = {};
    vector<string> actual = split("");
    assert(vector_equal(actual, expected) && "test_split_empty_string failed");
}

void test_split_only_whitespace() {
    vector<string> expected = {};
    vector<string> actual = split("     ");
    assert(vector_equal(actual, expected) && "test_split_only_whitespace failed");
}

void test_split_with_punctuation() {
    vector<string> expected = {"hello,", "world!"};
    vector<string> actual = split("hello, world!");
    assert(vector_equal(actual, expected) && "test_split_with_punctuation failed");
}

void test_split_mixed_whitespace() {
    vector<string> expected = {"hello", "world"};
    vector<string> actual = split("hello\nworld\t");
    assert(vector_equal(actual, expected) && "test_split_mixed_whitespace failed");
}

void test_split_numbers() {
    vector<string> expected = {"123", "456", "789"};
    vector<string> actual = split("123 456 789");
    assert(vector_equal(actual, expected) && "test_split_numbers failed");
}

void test_split_with_tabs_and_newlines() {
    vector<string> expected = {"hello", "world"};
    vector<string> actual = split("hello\n\tworld");
    assert(vector_equal(actual, expected) && "test_split_with_tabs_and_newlines failed");
}


TEST(test_split_single_word);
TEST(test_split_multiple_words);
TEST(test_split_with_extra_whitespace);
TEST(test_split_empty_string);
TEST(test_split_only_whitespace);
TEST(test_split_with_punctuation);
TEST(test_split_mixed_whitespace);
TEST(test_split_numbers);
TEST(test_split_with_tabs_and_newlines);
#endif
