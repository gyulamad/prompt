#pragma once

#include <string>
#include <map>

#include "../utils/ERROR.hpp"

using namespace std;
using namespace tools::utils;

namespace tools::str {

    string str_replace(const map<string, string>& v, const string& s) {
        // Create a modifiable copy of the input string
        string result = s;
        // Iterate through each key-value pair in the map
        for (const auto& pair : v) {
            if (pair.first == pair.second) continue;
            // Check if the key is empty
            if (pair.first.empty()) throw ERROR("Cannot replace from an empty string");
    
            size_t pos = 0;
            // Search for the key in the string and replace all occurrences
            while ((pos = result.find(pair.first, pos)) != string::npos) {
                result.replace(pos, pair.first.length(), pair.second);
                // Move past the replacement
                if (!pair.second.empty()) pos += pair.second.length(); // If the replacement is empty, do not increment pos
            }
        }
        // Return the modified string
        return result;
    }

    string str_replace(const string& from, const string& to, const string& subject) {
        // if (from.empty()) throw ERROR("Cannot replace from an empty string");
        return str_replace({{from, to}}, subject);
    }
    
}

#ifdef TEST

// #include "../utils/Test.hpp"

using namespace tools::str;

// Test cases for str_replace
void test_str_replace_single_replacement() {
    string input = "hello world";
    string expected = "hi world";
    string actual = str_replace("hello", "hi", input);
    assert(actual == expected && "test_str_replace_single_replacement failed");
}

void test_str_replace_multiple_occurrences() {
    string input = "apple banana apple";
    string expected = "fruit banana fruit";
    string actual = str_replace("apple", "fruit", input);
    assert(actual == expected && "test_str_replace_multiple_occurrences failed");
}

void test_str_replace_no_match() {
    string input = "hello world";
    string expected = "hello world";
    string actual = str_replace("foo", "bar", input);
    assert(actual == expected && "test_str_replace_no_match failed");
}

void test_str_replace_empty_to_multiple_occurrences() {
    string input = "apple banana apple";
    string expected = " banana ";
    string actual = str_replace("apple", "", input);
    assert(actual == expected && "test_str_replace_empty_to_multiple_occurrences failed");
}

void test_str_replace_empty_to_no_match() {
    string input = "hello world";
    string expected = "hello world";
    string actual = str_replace("foo", "", input);
    assert(actual == expected && "test_str_replace_empty_to_no_match failed");
}

void test_str_replace_non_empty_to() {
    string input = "hello world";
    string expected = "hello universe";
    string actual = str_replace("world", "universe", input);
    assert(actual == expected && "test_str_replace_non_empty_to failed");
}

void test_str_replace_empty_input() {
    string input = "";
    string expected = "";
    string actual = str_replace("foo", "bar", input);
    assert(actual == expected && "test_str_replace_empty_input failed");
}

void test_str_replace_empty_to() {
    string input = "hello world";
    string expected = "hello ";
    string actual = str_replace("world", "", input);
    assert(actual == expected && "test_str_replace_empty_to failed");
}

void test_str_replace_empty_from() {
    bool thrown = false;
    try {
        string input = "hello world";
        string actual = str_replace("", "bar", input);
    } catch (const exception&) {
        thrown = true;
    }
    assert(thrown && "test_str_replace_empty_from failed");
}

void test_str_replace_map_multiple_replacements() {
    string input = "hello world, hello universe";
    map<string, string> replacements = {{"hello", "hi"}, {"world", "earth"}};
    string expected = "hi earth, hi universe";
    string actual = str_replace(replacements, input);
    assert(actual == expected && "test_str_replace_map_multiple_replacements failed");
}

void test_str_replace_map_overlapping_keys() {
    string input = "abcde";
    map<string, string> replacements = {{"abc", "123"}, {"cde", "456"}};
    string expected = "123de";
    string actual = str_replace(replacements, input);
    assert(actual == expected && "test_str_replace_map_overlapping_keys failed");
}

void test_str_replace_map_empty_input() {
    string input = "";
    map<string, string> replacements = {{"hello", "hi"}, {"world", "earth"}};
    string expected = "";
    string actual = str_replace(replacements, input);
    assert(actual == expected && "test_str_replace_map_empty_input failed");
}

TEST(test_str_replace_single_replacement);
TEST(test_str_replace_multiple_occurrences);
TEST(test_str_replace_no_match);
TEST(test_str_replace_empty_to_multiple_occurrences);
TEST(test_str_replace_empty_to_no_match);
TEST(test_str_replace_non_empty_to);
TEST(test_str_replace_empty_input);
TEST(test_str_replace_empty_to);
TEST(test_str_replace_empty_from);
TEST(test_str_replace_map_multiple_replacements);
TEST(test_str_replace_map_overlapping_keys);
TEST(test_str_replace_map_empty_input);
#endif
