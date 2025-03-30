#pragma once

#include <vector>

#include "../utils/foreach.hpp"

using namespace std;

using namespace tools::utils;

namespace tools::containers {

    // Function to compute difference between two vectors (values in vec1 not in vec2)
    template <typename T>
    vector<T> array_diff(const vector<T>& vec1, const vector<T>& vec2) {
        vector<T> results;
        
        foreach (vec1, [&](const T& item) {
            if (!in_array(item, vec2)) results.push_back(item);
        });
        
        return results;
    }
    
}

#ifdef TEST

using namespace tools::containers;

void test_array_diff_basic() {
    vector<int> vec1 = {1, 2, 3, 4};
    vector<int> vec2 = {3, 4, 5};
    auto result = array_diff(vec1, vec2);
    vector<int> expected = {1, 2};
    assert(vector_equal(result, expected) && "Basic array_diff test failed");
}

void test_array_diff_empty_first() {
    vector<string> vec1 = {};
    vector<string> vec2 = {"a", "b", "c"};
    auto result = array_diff(vec1, vec2);
    assert(result.empty() && "Empty first vector should return empty result");
}

void test_array_diff_empty_second() {
    vector<double> vec1 = {1.1, 2.2, 3.3};
    vector<double> vec2 = {};
    auto result = array_diff(vec1, vec2);
    vector<double> expected = {1.1, 2.2, 3.3};
    assert(vector_equal(result, expected) && "Empty second vector should return copy of first");
}

void test_array_diff_no_overlap() {
    vector<char> vec1 = {'a', 'b', 'c'};
    vector<char> vec2 = {'x', 'y', 'z'};
    auto result = array_diff(vec1, vec2);
    vector<char> expected = {'a', 'b', 'c'};
    assert(vector_equal(result, expected) && "No overlap should return copy of first vector");
}

void test_array_diff_complete_overlap() {
    vector<int> vec1 = {10, 20, 30};
    vector<int> vec2 = {10, 20, 30};
    auto result = array_diff(vec1, vec2);
    assert(result.empty() && "Complete overlap should return empty vector");
}

void test_array_diff_duplicates() {
    vector<int> vec1 = {1, 2, 2, 3, 4, 4};
    vector<int> vec2 = {2, 4};
    auto result = array_diff(vec1, vec2);
    vector<int> expected = {1, 3};
    assert(vector_equal(result, expected) && "Should remove all occurrences of values");
}

void test_array_diff_custom_type() {
    struct Point { int x, y; bool operator==(const Point& p) const { return x == p.x && y == p.y; } };
    vector<Point> vec1 = {{1,1}, {2,2}, {3,3}};
    vector<Point> vec2 = {{2,2}};
    auto result = array_diff(vec1, vec2);
    vector<Point> expected = {{1,1}, {3,3}};
    assert(vector_equal(result, expected) && "Should work with custom types that implement ==");
}

// Register tests
TEST(test_array_diff_basic);
TEST(test_array_diff_empty_first);
TEST(test_array_diff_empty_second);
TEST(test_array_diff_no_overlap);
TEST(test_array_diff_complete_overlap);
TEST(test_array_diff_duplicates);
TEST(test_array_diff_custom_type);

#endif
