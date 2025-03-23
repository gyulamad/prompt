#pragma once

#include <vector>

using namespace std;

namespace tools::containers {

    template<typename T>
    vector<T> array_reverse(vector<T> array) {
        // Handle empty vector case
        if (array.empty()) return array;
        
        vector<T> reversed;
        reversed.reserve(array.size());
        
        // Insert elements in reverse order
        for (auto it = array.rbegin(); it != array.rend(); ++it) {
            reversed.push_back(*it);
        }
        
        return reversed;
    }
    
}

#ifdef TEST

using namespace tools::containers;
// Test cases for array_reverse
void test_array_reverse_empty() {
    vector<int> arr;
    vector<int> reversed = array_reverse(arr);
    assert(reversed.empty() && "Empty array should remain empty after reverse");
}

void test_array_reverse_single_element() {
    vector<int> arr = {1};
    vector<int> reversed = array_reverse(arr);
    vector<int> expected = {1};
    assert(reversed == expected && "Single-element array should remain the same after reverse");
}

void test_array_reverse_even_elements() {
    vector<int> arr = {1, 2, 3, 4};
    vector<int> reversed = array_reverse(arr);
    vector<int> expected = {4, 3, 2, 1};
    assert(reversed == expected && "Even-element array should be reversed");
}

void test_array_reverse_odd_elements() {
    vector<int> arr = {1, 2, 3, 4, 5};
    vector<int> reversed = array_reverse(arr);
    vector<int> expected = {5, 4, 3, 2, 1};
    assert(reversed == expected && "Odd-element array should be reversed");
}

void test_array_reverse_strings() {
    vector<string> arr = {"a", "b", "c", "d"};
    vector<string> reversed = array_reverse(arr);
    vector<string> expected = {"d", "c", "b", "a"};
    assert(reversed == expected && "String array should be reversed");
}

void test_array_reverse_large_array() {
    vector<int> arr;
    for (int i = 0; i < 1000; ++i) {
        arr.push_back(i);
    }
    vector<int> reversed = array_reverse(arr);
    vector<int> expected;
    for (int i = 999; i >= 0; --i) {
        expected.push_back(i);
    }
    assert(reversed == expected && "Large array should be reversed");
}

void test_array_reverse_custom_objects() {
    struct Point {
        int x, y;
        bool operator==(const Point& other) const {
            return x == other.x && y == other.y;
        }
    };
    vector<Point> arr = {{1, 2}, {3, 4}, {5, 6}};
    vector<Point> reversed = array_reverse(arr);
    vector<Point> expected = {{5, 6}, {3, 4}, {1, 2}};
    assert(reversed == expected && "Custom object array should be reversed");
}

void test_array_reverse_alloc_fail() {
    vector<int> arr;
    for (int i=0; i<10; i++) arr.push_back(i);
    bool exception_thrown = false;

    // Instead of trying to exhaust memory, let's modify the array_reverse function
    // temporarily for this test. This requires a bit of redesign.

    // Option 1: Use a global flag that the array_reverse function can check
    g_simulate_memory_failure = true;
    
    try {
        vector<int> result = array_reverse(arr);
    } catch (const bad_alloc& e) {
        exception_thrown = true;
    } catch (const exception& e) {
        // Other exceptions
    }
    
    g_simulate_memory_failure = false;
    
    assert(exception_thrown && "Memory allocation error not handled in array_reverse");
}

TEST(test_array_reverse_empty);
TEST(test_array_reverse_single_element);
TEST(test_array_reverse_even_elements);
TEST(test_array_reverse_odd_elements);
TEST(test_array_reverse_strings);
TEST(test_array_reverse_large_array);
TEST(test_array_reverse_custom_objects);
TEST(test_array_reverse_alloc_fail);
#endif
