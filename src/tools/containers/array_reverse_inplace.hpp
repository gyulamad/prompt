#pragma once

#include <vector>

using namespace std;

namespace tools::containers {

    template<typename T>
    void array_reverse_inplace(vector<T>& array) {
        if (array.empty()) return;
           
        size_t start = 0;
        size_t end = array.size() - 1;
        
        while (start < end) {
            swap(array[start], array[end]);
            start++;
            end--;
        }
    }
    
}

#ifdef TEST

using namespace tools::containers;

// Test cases for array_reverse_inplace
void test_array_reverse_inplace_empty() {
    vector<int> arr;
    array_reverse_inplace(arr);
    assert(arr.empty() && "Empty array should remain empty after in-place reverse");
}

void test_array_reverse_inplace_single_element() {
    vector<int> arr = {1};
    array_reverse_inplace(arr);
    vector<int> expected = {1};
    assert(arr == expected && "Single-element array should remain the same after in-place reverse");
}

void test_array_reverse_inplace_even_elements() {
    vector<int> arr = {1, 2, 3, 4};
    array_reverse_inplace(arr);
    vector<int> expected = {4, 3, 2, 1};
    assert(arr == expected && "Even-element array should be reversed in place");
}

void test_array_reverse_inplace_odd_elements() {
    vector<int> arr = {1, 2, 3, 4, 5};
    array_reverse_inplace(arr);
    vector<int> expected = {5, 4, 3, 2, 1};
    assert(arr == expected && "Odd-element array should be reversed in place");
}

void test_array_reverse_inplace_strings() {
    vector<string> arr = {"a", "b", "c", "d"};
    array_reverse_inplace(arr);
    vector<string> expected = {"d", "c", "b", "a"};
    assert(arr == expected && "String array should be reversed in place");
}

void test_array_reverse_inplace_large_array() {
    vector<int> arr;
    for (int i = 0; i < 1000; ++i) {
        arr.push_back(i);
    }
    array_reverse_inplace(arr);
    vector<int> expected;
    for (int i = 999; i >= 0; --i) {
        expected.push_back(i);
    }
    assert(arr == expected && "Large array should be reversed in place");
}

void test_array_reverse_inplace_custom_objects() {
    struct Point {
        int x, y;
        bool operator==(const Point& other) const {
            return x == other.x && y == other.y;
        }
    };
    vector<Point> arr = {{1, 2}, {3, 4}, {5, 6}};
    array_reverse_inplace(arr);
    vector<Point> expected = {{5, 6}, {3, 4}, {1, 2}};
    assert(arr == expected && "Custom object array should be reversed in place");
}

void test_array_reverse_inplace_alloc_fail() {
    vector<int> arr;
    for (int i=0; i<10; i++) arr.push_back(i);
    bool exception_thrown = false;
    
    // Create a mock that simulates a memory allocation failure during reversal
    try {
        // We need to modify array_reverse_inplace to simulate the allocation failure
        // For testing purposes, let's use a global flag or function parameter
        
        // Call the function with a flag or in a context that will cause it to simulate a failure
        g_simulate_memory_failure = true;  // Global flag approach
        array_reverse_inplace(arr);
        g_simulate_memory_failure = false;
    } catch (const bad_alloc& e) {
        exception_thrown = true;
    } catch (const exception& e) {
        exception_thrown = true;  // Also catch other exceptions
    }
    
    assert(exception_thrown && "Memory allocation error not handled in array_reverse_inplace");
}

TEST(test_array_reverse_inplace_empty);
TEST(test_array_reverse_inplace_single_element);
TEST(test_array_reverse_inplace_even_elements);
TEST(test_array_reverse_inplace_odd_elements);
TEST(test_array_reverse_inplace_strings);
TEST(test_array_reverse_inplace_large_array);
TEST(test_array_reverse_inplace_custom_objects);
TEST(test_array_reverse_inplace_alloc_fail);
#endif
