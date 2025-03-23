#pragma once

#include <vector>
#include <functional>

using namespace std;

namespace tools::containers {

    template<typename T>
    vector<T> array_filter(const vector<T>& input, function<bool(const T&)> predicate = [](const T& val) { 
        return !val.empty(); 
    }) {
        vector<T> result;
        copy_if(input.begin(), input.end(), back_inserter(result), predicate);
        return result;
    }
    
}

#ifdef TEST

using namespace tools::containers;

void test_array_filter_basic() {
    vector<int> input = {1, 2, 3, 4, 5};
    function<bool(const int&)> predicate = [](const int& val) { return val % 2 == 0; }; // Keep even numbers
    vector<int> result = array_filter(input, predicate);
    vector<int> expected = {2, 4};
    assert(result == expected && "Basic filter (even numbers)");
}

void test_array_filter_empty_input() {
    vector<int> input;
    function<bool(const int&)> predicate = [](const int& val) { return val > 0; }; // Keep positive numbers
    vector<int> result = array_filter(input, predicate);
    assert(result.empty() && "Empty input");
}

void test_array_filter_default_predicate() {
    vector<string> input = {"hello", "", "world", "", "!"};
    vector<string> result = array_filter(input); // Use default predicate (!val.empty())
    vector<string> expected = {"hello", "world", "!"};
    assert(result == expected && "Default predicate (non-empty strings)");
}

void test_array_filter_all_elements_filtered() {
    vector<int> input = {1, 3, 5, 7};
    function<bool(const int&)> predicate = [](const int& val) { return val % 2 == 0; }; // Keep even numbers
    vector<int> result = array_filter(input, predicate);
    assert(result.empty() && "All elements filtered out");
}

void test_array_filter_custom_objects() {
    struct Point {
        int x, y;
        bool operator==(const Point& other) const {
            return x == other.x && y == other.y;
        }
    };
    vector<Point> input = {{1, 2}, {3, 4}, {5, 6}, {7, 8}};
    function<bool(const Point&)> predicate = [](const Point& p) { return p.x + p.y > 8; }; // Keep points where x + y > 8
    vector<Point> result = array_filter(input, predicate);
    vector<Point> expected = {{5, 6}, {7, 8}};
    assert(result == expected && "Filter custom objects");
}

void test_array_filter_no_elements_filtered() {
    vector<int> input = {2, 4, 6, 8};
    function<bool(const int&)> predicate = [](const int& val) { return val % 2 == 0; }; // Keep even numbers
    vector<int> result = array_filter(input, predicate);
    vector<int> expected = {2, 4, 6, 8};
    assert(result == expected && "No elements filtered out");
}

TEST(test_array_filter_basic);
TEST(test_array_filter_empty_input);
TEST(test_array_filter_default_predicate);
TEST(test_array_filter_all_elements_filtered);
TEST(test_array_filter_custom_objects);
TEST(test_array_filter_no_elements_filtered);
#endif
