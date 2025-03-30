#pragma once

#include <string>
#include <vector>

#include "parse.hpp"

using namespace std;

namespace tools::str {

    template<typename T>
    vector<T> parse_vector(const string& input, const string& sep = ",") {
        vector<string> splits = explode(sep, input);
        vector<T> results;
        foreach (splits, [&](const string& split) {
            results.push_back(parse<T>(split));
        });
        return results;
    }

}

#ifdef TEST

using namespace tools::str;

void test_parse_vector_int_basic() {
    string input = "1,2,3,4";
    auto result = parse_vector<int>(input);
    vector<int> expected = {1, 2, 3, 4};
    assert(vector_equal(result, expected) && "Basic int vector parsing failed");
}

void test_parse_vector_int_spaces() {
    string input = " 1 , 2 , 3 , 4 ";
    auto result = parse_vector<int>(input);
    vector<int> expected = {1, 2, 3, 4};
    assert(vector_equal(result, expected) && "Int vector with spaces parsing failed");
}

void test_parse_vector_double() {
    string input = "1.5,2.25,3.75";
    auto result = parse_vector<double>(input);
    vector<double> expected = {1.5, 2.25, 3.75};
    assert(vector_equal(result, expected) && "Double vector parsing failed");
}

void test_parse_vector_string() {
    string input = "apple,banana,cherry";
    auto result = parse_vector<string>(input);
    vector<string> expected = {"apple", "banana", "cherry"};
    assert(vector_equal(result, expected) && "String vector parsing failed");
}

void test_parse_vector_custom_separator() {
    string input = "1|2|3|4";
    auto result = parse_vector<int>(input, "|");
    vector<int> expected = {1, 2, 3, 4};
    assert(vector_equal(result, expected) && "Custom separator parsing failed");
}

void test_parse_vector_empty_input() {
    string input = "";
    bool throws = false;
    try {
        parse_vector<int>(input);
    } catch (exception& e) {
        throws = true;
        string what = e.what();
        assert(str_contains(what, "Invalid input string (not a number): <empty>"));
    }
    assert(throws && "Empty input should throw as it's not a number");
}

void test_parse_vector_single_item() {
    string input = "42";
    auto result = parse_vector<int>(input);
    vector<int> expected = {42};
    assert(vector_equal(result, expected) && "Single item parsing failed");
}

void test_parse_vector_invalid_input() {
    string input = "1,apple,3";
    bool thrown = false;
    try {
        parse_vector<int>(input);
    } catch (exception& e) {
        thrown = true;
        string what = e.what();
        assert(str_contains(what, "Invalid input string (not a number): apple") && "Exception message should contains");
    }
    assert(thrown && "Should throw on invalid input");
}

void test_parse_vector_bool() {
    string input = "true,false,1,0";
    auto result = parse_vector<bool>(input);
    vector<bool> expected = {true, false, true, false};
    assert(vector_equal(result, expected) && "Bool vector parsing failed");
}

// Register tests
TEST(test_parse_vector_int_basic);
TEST(test_parse_vector_int_spaces);
TEST(test_parse_vector_double);
TEST(test_parse_vector_string);
TEST(test_parse_vector_custom_separator);
TEST(test_parse_vector_empty_input);
TEST(test_parse_vector_single_item);
TEST(test_parse_vector_invalid_input);
TEST(test_parse_vector_bool);

#endif