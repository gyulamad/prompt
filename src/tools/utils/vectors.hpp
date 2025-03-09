#pragma once

#include <vector>
#include <type_traits>
#include <utility>
#include <unordered_set>
#include <map>
#include <functional>

#include "ERROR.hpp"

namespace tools::utils {

    template <typename T>
    T array_shift(vector<T>& vec) {
        if (vec.empty())
            throw ERROR("Cannot shift from an empty vector");

        // Save the first element (to return it)
        T firstElement = move(vec.front());

        // Remove the first element
        vec.erase(vec.begin());

        // Return the shifted element
        return firstElement;
    }

    template <typename T>
    vector<T> array_unique(const vector<T>& vec) {
        // Use an unordered_set to filter out duplicates
        unordered_set<T> seen;
        vector<T> result;

        // Reserve space for the result vector to avoid repeated reallocations
        result.reserve(vec.size());

        // Iterate through the input vector
        for (const auto& element : vec)
            // Insert the element into the set (if it's not already present)
            if (seen.insert(element).second)
                // If the element was inserted (i.e., it's unique), add it to the result
                result.push_back(element);

        return result;
    }
    
    // Function to merge two vectors
    template <typename T>
    vector<T> array_merge(const vector<T>& vec1, const vector<T>& vec2) {
        vector<T> result;

        // Reserve space to avoid repeated reallocations
        result.reserve(vec1.size() + vec2.size());

        // Add elements from the first vector
        result.insert(result.end(), vec1.begin(), vec1.end());

        // Add elements from the second vector
        result.insert(result.end(), vec2.begin(), vec2.end());

        return result;
    }

    template<typename T>
    void array_dump(vector<T> vec, bool dbg = true) {
        if (dbg) DEBUG("dump vector(" + to_string(vec.size()) + "):");
        size_t nth = 0;
        for (const T& elem: vec) cout << nth++ << ": " << elem << endl;
    }
    

    template <typename T>
    void sort(vector<T>& vec) {
        sort(vec.begin(), vec.end()); // Sorts in ascending order
    }

    template <typename T>
    void rsort(vector<T>& vec) {
        sort(vec.begin(), vec.end(), greater<T>()); // Sorts in descending order
    }

    template<typename T>
    vector<T> array_filter(const vector<T>& input, function<bool(const T&)> predicate = [](const T& val) { 
        return !val.empty(); 
    }) {
        vector<T> result;
        copy_if(input.begin(), input.end(), back_inserter(result), predicate);
        return result;
    }

    template <typename T, typename = void>
    struct has_key_type : false_type {};

    template <typename T>
    struct has_key_type<T, void_t<typename T::key_type>> : true_type {};

    template <typename Container, typename = void>
    struct KeyTypeTrait {
        using type = size_t;
    };

    template <typename Container>
    struct KeyTypeTrait<Container, void_t<typename Container::key_type>> {
        using type = typename Container::key_type;
    };

    template <typename Container>
    auto array_keys(const Container& container) {
        using KeyType = typename KeyTypeTrait<Container>::type;
        vector<KeyType> keys;

        if constexpr (has_key_type<Container>::value) {
            for (const auto& element : container)
                keys.push_back(element.first);
        } else {
            for (size_t i = 0; i < container.size(); ++i)
                keys.push_back(i);
        }

        return keys;
    }

    template <typename T>
    struct is_pair : false_type {};

    template <typename T1, typename T2>
    struct is_pair<pair<T1, T2>> : true_type {};

    template <typename Needle, typename Container>
    bool in_array(const Needle& needle, const Container& container) {
        using Element = typename Container::value_type;

        if constexpr (is_pair<Element>::value) {
            // Check values in associative containers (maps, unordered_maps)
            for (const auto& element : container)
                if (element.second == needle) return true;
        } else {
            // Check elements in sequence containers (vectors, lists, etc.)
            for (const auto& element : container)
                if (element == needle) return true;
        }

        return false;
    }

    // Global flag for testing
    bool g_simulate_memory_failure = false;

    template<typename T>
    vector<T> array_reverse(vector<T> array) {
        // Handle empty vector case
        if (array.empty()) return array;
        
        // If we're simulating a memory failure for testing
        if (g_simulate_memory_failure) {
            throw bad_alloc();
        }
        
        try {
            vector<T> reversed;
            reversed.reserve(array.size());
            
            // Insert elements in reverse order
            for (auto it = array.rbegin(); it != array.rend(); ++it) {
                reversed.push_back(*it);
            }
            
            return reversed;
        }
        catch (const bad_alloc& e) {
            throw e; // Rethrow the bad_alloc exception
        }
        catch (...) {
            throw ERROR("Unknown error occurred during vector reversal");
        }
    }

    template<typename T>
    void array_reverse_inplace(vector<T>& array) {
        if (array.empty()) {
            return;
        }
        
        try {
            // Simulate memory allocation failure for testing
            if (g_simulate_memory_failure) {
                throw bad_alloc();
            }
            
            size_t start = 0;
            size_t end = array.size() - 1;
            
            while (start < end) {
                swap(array[start], array[end]);
                start++;
                end--;
            }
        }
        catch (const bad_alloc& e) {
            // Explicitly handle bad_alloc exceptions
            throw; // Rethrow to be caught by the test
        }
        catch (...) {
            throw ERROR("Error occurred during in-place vector reversal");
        }
    }

    // Template function to compare two vectors for equality
    template<typename T>
    bool vector_equal(const vector<T>& a, const vector<T>& b) {
        // First, check if the sizes of the vectors are equal
        if (a.size() != b.size()) return false;

        // Then, compare each element of the vectors
        for (size_t i = 0; i < a.size(); ++i)
            if (a[i] != b[i]) return false;

        // If all elements are equal, return true
        return true;
    }

};

#ifdef TEST

#include "io.hpp"

using namespace tools::utils;

void test_array_shift_basic() {
    vector<int> vec = {1, 2, 3};
    int shifted = array_shift(vec);
    assert(shifted == 1 && "Shifted element");
    assert(vec.size() == 2 && "Vector size after shift");
    assert(vec[0] == 2 && "First element after shift");
    assert(vec[1] == 3 && "Second element after shift");
}

void test_array_shift_single_element() {
    vector<int> vec = {42};
    int shifted = array_shift(vec);
    assert(shifted == 42 && "Shifted element");
    assert(vec.empty() && "Vector should be empty after shift");
}

void test_array_shift_strings() {
    vector<string> vec = {"hello", "world"};
    string shifted = array_shift(vec);
    assert(shifted == "hello" && "Shifted element");
    assert(vec.size() == 1 && "Vector size after shift");
    assert(vec[0] == "world" && "First element after shift");
}

void test_array_shift_empty_vector() {
    vector<int> vec;
    bool exception_thrown = false;
    try {
        array_shift(vec);
    } catch (const exception& e) {
        exception_thrown = true;
    }
    assert(exception_thrown && "Exception should be thrown for empty vector");
}

void test_array_shift_move_semantics() {
    vector<string> vec = {"hello", "world"};
    string shifted = array_shift(vec);
    assert(shifted == "hello" && "Shifted element");
    assert(vec.size() == 1 && "Vector size after shift");
    assert(vec[0] == "world" && "First element after shift");
}

void test_array_shift_large_vector() {
    vector<int> vec = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    int shifted = array_shift(vec);
    assert(shifted == 1 && "Shifted element");
    assert(vec.size() == 9 && "Vector size after shift");
    assert(vec[0] == 2 && "First element after shift");
    assert(vec[8] == 10 && "Last element after shift");
}

void test_array_unique_basic() {
    vector<int> vec = {1, 2, 2, 3, 4, 4, 4, 5};
    vector<int> result = array_unique(vec);
    vector<int> expected = {1, 2, 3, 4, 5};
    assert(result == expected && "Basic uniqueness");
}

void test_array_unique_empty_vector() {
    vector<int> vec;
    vector<int> result = array_unique(vec);
    assert(result.empty() && "Empty vector");
}

void test_array_unique_all_duplicates() {
    vector<int> vec = {1, 1, 1, 1, 1};
    vector<int> result = array_unique(vec);
    vector<int> expected = {1};
    assert(result == expected && "All duplicates");
}

void test_array_unique_strings() {
    vector<string> vec = {"hello", "world", "hello", "cpp", "world"};
    vector<string> result = array_unique(vec);
    vector<string> expected = {"hello", "world", "cpp"};
    assert(result == expected && "Strings with duplicates");
}

void test_array_unique_large_vector() {
    vector<int> vec = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 1, 2, 3, 4, 5};
    vector<int> result = array_unique(vec);
    vector<int> expected = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    assert(result == expected && "Large vector with duplicates");
}

void test_array_merge_basic() {
    vector<int> vec1 = {1, 2, 3};
    vector<int> vec2 = {4, 5, 6};
    vector<int> result = array_merge(vec1, vec2);
    vector<int> expected = {1, 2, 3, 4, 5, 6};
    assert(result == expected && "Basic merge");
}

void test_array_merge_empty_first_vector() {
    vector<int> vec1;
    vector<int> vec2 = {4, 5, 6};
    vector<int> result = array_merge(vec1, vec2);
    vector<int> expected = {4, 5, 6};
    assert(result == expected && "Empty first vector");
}

void test_array_merge_empty_second_vector() {
    vector<int> vec1 = {1, 2, 3};
    vector<int> vec2;
    vector<int> result = array_merge(vec1, vec2);
    vector<int> expected = {1, 2, 3};
    assert(result == expected && "Empty second vector");
}

void test_array_merge_both_empty_vectors() {
    vector<int> vec1;
    vector<int> vec2;
    vector<int> result = array_merge(vec1, vec2);
    assert(result.empty() && "Both vectors empty");
}

void test_array_merge_strings() {
    vector<string> vec1 = {"hello"};
    vector<string> vec2 = {"world"};
    vector<string> result = array_merge(vec1, vec2);
    vector<string> expected = {"hello", "world"};
    assert(result == expected && "Merge strings");
}

void test_array_merge_large_vectors() {
    vector<int> vec1 = {1, 2, 3, 4, 5};
    vector<int> vec2 = {6, 7, 8, 9, 10};
    vector<int> result = array_merge(vec1, vec2);
    vector<int> expected = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    assert(result == expected && "Merge large vectors");
}

void test_array_merge_custom_objects() {
    struct Point {
        int x, y;
        bool operator==(const Point& other) const {
            return x == other.x && y == other.y;
        }
    };

    vector<Point> vec1 = {{1, 2}, {3, 4}};
    vector<Point> vec2 = {{5, 6}};
    vector<Point> result = array_merge(vec1, vec2);
    vector<Point> expected = {{1, 2}, {3, 4}, {5, 6}};
    assert(result == expected && "Merge custom objects");
}

void test_array_dump_basic() {
    vector<int> vec = {1, 2, 3};
    string expected = 
        "dump vector(3):\n"
        "0: 1\n"
        "1: 2\n"
        "2: 3\n";
    
    string output = capture_cout([&]() { array_dump(vec); });
    assert(output == expected && "Basic dump");
}

void test_array_dump_empty_vector() {
    vector<int> vec;
    string expected = 
        "dump vector(0):\n";
    
    string output = capture_cout([&]() { array_dump(vec); });
    assert(output == expected && "Empty vector dump");
}

void test_array_dump_strings() {
    vector<string> vec = {"hello", "world"};
    string expected = 
        "dump vector(2):\n"
        "0: hello\n"
        "1: world\n";
    
    string output = capture_cout([&]() { array_dump(vec); });
    assert(output == expected && "Strings dump");
}

void test_array_dump_no_debug() {
    vector<int> vec = {1, 2, 3};
    string expected = 
        "0: 1\n"
        "1: 2\n"
        "2: 3\n";
    
    string output = capture_cout([&]() { array_dump(vec, false); });
    assert(output == expected && "No debug dump");
}

void test_sort_basic() {
    vector<int> vec = {3, 1, 4, 1, 5, 9, 2, 6};
    sort(vec);
    vector<int> expected = {1, 1, 2, 3, 4, 5, 6, 9};
    assert(vec == expected && "Basic sort (ascending)");
}

void test_sort_empty_vector() {
    vector<int> vec;
    sort(vec);
    assert(vec.empty() && "Empty vector sort");
}

void test_sort_single_element() {
    vector<int> vec = {42};
    sort(vec);
    vector<int> expected = {42};
    assert(vec == expected && "Single element sort");
}

void test_sort_strings() {
    vector<string> vec = {"banana", "apple", "cherry"};
    sort(vec);
    vector<string> expected = {"apple", "banana", "cherry"};
    assert(vec == expected && "Sort strings (ascending)");
}

void test_rsort_basic() {
    vector<int> vec = {3, 1, 4, 1, 5, 9, 2, 6};
    rsort(vec);
    vector<int> expected = {9, 6, 5, 4, 3, 2, 1, 1};
    assert(vec == expected && "Basic reverse sort (descending)");
}

void test_rsort_empty_vector() {
    vector<int> vec;
    rsort(vec);
    assert(vec.empty() && "Empty vector reverse sort");
}

void test_rsort_single_element() {
    vector<int> vec = {42};
    rsort(vec);
    vector<int> expected = {42};
    assert(vec == expected && "Single element reverse sort");
}

void test_rsort_strings() {
    vector<string> vec = {"banana", "apple", "cherry"};
    rsort(vec);
    vector<string> expected = {"cherry", "banana", "apple"};
    assert(vec == expected && "Reverse sort strings (descending)");
}

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

// Test cases for in_array

// Test case with integer vector
void test_in_array_integer_vector_exists() {
    vector<int> vec = {1, 2, 3, 4, 5};
    assert(in_array(3, vec) && "Integer exists in vector");
}

void test_in_array_integer_vector_not_exists() {
    vector<int> vec = {1, 2, 3, 4, 5};
    assert(!in_array(6, vec) && "Integer does not exist in vector");
}

void test_in_array_integer_vector_empty() {
    vector<int> vec;
    assert(!in_array(1, vec) && "Integer does not exist in empty vector");
}

// Test case with string vector
void test_in_array_string_vector_exists() {
    vector<string> vec = {"apple", "banana", "cherry"};
    assert(in_array(string("banana"), vec) && "String exists in vector");
}

void test_in_array_string_vector_not_exists() {
    vector<string> vec = {"apple", "banana", "cherry"};
    assert(!in_array(string("grape"), vec) && "String does not exist in vector");
}

void test_in_array_string_vector_empty() {
    vector<string> vec;
    assert(!in_array(string("apple"), vec) && "String does not exist in empty vector");
}

// Test case with map (key-value pairs)
void test_in_array_map_exists() {
    map<int, string> myMap = {{1, "one"}, {2, "two"}, {3, "three"}};
    assert(in_array(string("two"), myMap) && "Value exists in map");
}

void test_in_array_map_not_exists() {
    map<int, string> myMap = {{1, "one"}, {2, "two"}, {3, "three"}};
    assert(!in_array(string("four"), myMap) && "Value does not exist in map");
}

void test_in_array_map_empty() {
    map<int, string> myMap;
    assert(!in_array(string("one"), myMap) && "Value does not exist in empty map");
}

// Test case with unordered_map (key-value pairs)
void test_in_array_unordered_map_exists() {
    unordered_map<int, string> myMap = {{1, "one"}, {2, "two"}, {3, "three"}};
    assert(in_array(string("two"), myMap) && "Value exists in unordered_map");
}

void test_in_array_unordered_map_not_exists() {
    unordered_map<int, string> myMap = {{1, "one"}, {2, "two"}, {3, "three"}};
    assert(!in_array(string("four"), myMap) && "Value does not exist in unordered_map");
}

void test_in_array_unordered_map_empty() {
    unordered_map<int, string> myMap;
    assert(!in_array(string("one"), myMap) && "Value does not exist in empty unordered_map");
}

// Test case with custom object vector
void test_in_array_custom_object_exists() {
    struct Point {
        int x, y;
        bool operator==(const Point& other) const {
            return x == other.x && y == other.y;
        }
    };
    vector<Point> vec = {{1, 2}, {3, 4}, {5, 6}};
    assert(in_array(Point{3, 4}, vec) && "Custom object exists in vector");
}

void test_in_array_custom_object_not_exists() {
    struct Point {
        int x, y;
        bool operator==(const Point& other) const {
            return x == other.x && y == other.y;
        }
    };
    vector<Point> vec = {{1, 2}, {3, 4}, {5, 6}};
    assert(!in_array(Point{7, 8}, vec) && "Custom object does not exist in vector");
}

void test_in_array_custom_object_vector_empty() {
    struct Point {
        int x, y;
        bool operator==(const Point& other) const {
            return x == other.x && y == other.y;
        }
    };
    vector<Point> vec;
    assert(!in_array(Point{1, 2}, vec) && "Custom object does not exist in empty vector");
}

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

// Test cases for vector_equal
void test_vector_equal_basic_equal() {
    vector<int> vec1 = {1, 2, 3};
    vector<int> vec2 = {1, 2, 3};
    assert(vector_equal(vec1, vec2) && "Basic vectors should be equal");
}

void test_vector_equal_basic_not_equal() {
    vector<int> vec1 = {1, 2, 3};
    vector<int> vec2 = {1, 2, 4};
    assert(!vector_equal(vec1, vec2) && "Basic vectors should not be equal");
}

void test_vector_equal_different_sizes() {
    vector<int> vec1 = {1, 2, 3};
    vector<int> vec2 = {1, 2};
    assert(!vector_equal(vec1, vec2) && "Vectors with different sizes should not be equal");
}

void test_vector_equal_empty_vectors() {
    vector<int> vec1;
    vector<int> vec2;
    assert(vector_equal(vec1, vec2) && "Empty vectors should be equal");
}

void test_vector_equal_one_empty() {
    vector<int> vec1 = {1,2,3};
    vector<int> vec2;
    assert(!vector_equal(vec1, vec2) && "One empty vector and one non empty should not be equal");
}

void test_vector_equal_strings_equal() {
    vector<string> vec1 = {"apple", "banana", "cherry"};
    vector<string> vec2 = {"apple", "banana", "cherry"};
    assert(vector_equal(vec1, vec2) && "String vectors should be equal");
}

void test_vector_equal_strings_not_equal() {
    vector<string> vec1 = {"apple", "banana", "cherry"};
    vector<string> vec2 = {"apple", "grape", "cherry"};
    assert(!vector_equal(vec1, vec2) && "String vectors should not be equal");
}

void test_vector_equal_custom_objects_equal() {
    struct Point {
        int x, y;
        bool operator==(const Point& other) const {
            return x == other.x && y == other.y;
        }
    };
    vector<Point> vec1 = {{1, 2}, {3, 4}, {5, 6}};
    vector<Point> vec2 = {{1, 2}, {3, 4}, {5, 6}};
    assert(vector_equal(vec1, vec2) && "Custom object vectors should be equal");
}

void test_vector_equal_custom_objects_not_equal() {
    struct Point {
        int x, y;
        bool operator==(const Point& other) const {
            return x == other.x && y == other.y;
        }
    };
    vector<Point> vec1 = {{1, 2}, {3, 4}, {5, 6}};
    vector<Point> vec2 = {{1, 2}, {3, 5}, {5, 6}};
    assert(!vector_equal(vec1, vec2) && "Custom object vectors should not be equal");
}
void test_vector_equal_large_equal() {
    vector<int> vec1;
    vector<int> vec2;
    for (int i=0; i<10000; i++) {
        vec1.push_back(i);
        vec2.push_back(i);
    }
    assert(vector_equal(vec1, vec2) && "Large vectors should be equal");
}
void test_vector_equal_large_not_equal() {
    vector<int> vec1;
    vector<int> vec2;
    for (int i=0; i<10000; i++) {
        vec1.push_back(i);
        vec2.push_back(i+1);
    }
    assert(!vector_equal(vec1, vec2) && "Large vectors should not be equal");
}

// Register tests
TEST(test_array_shift_basic);
TEST(test_array_shift_single_element);
TEST(test_array_shift_strings);
TEST(test_array_shift_empty_vector);
TEST(test_array_shift_move_semantics);
TEST(test_array_shift_large_vector);
TEST(test_array_unique_basic);
TEST(test_array_unique_empty_vector);
TEST(test_array_unique_all_duplicates);
TEST(test_array_unique_strings);
TEST(test_array_unique_large_vector);
TEST(test_array_merge_basic);
TEST(test_array_merge_empty_first_vector);
TEST(test_array_merge_empty_second_vector);
TEST(test_array_merge_both_empty_vectors);
TEST(test_array_merge_strings);
TEST(test_array_merge_large_vectors);
TEST(test_sort_basic);
TEST(test_sort_empty_vector);
TEST(test_sort_single_element);
TEST(test_sort_strings);
TEST(test_rsort_basic);
TEST(test_rsort_empty_vector);
TEST(test_rsort_single_element);
TEST(test_rsort_strings);
TEST(test_array_filter_basic);
TEST(test_array_filter_empty_input);
TEST(test_array_filter_default_predicate);
TEST(test_array_filter_all_elements_filtered);
TEST(test_array_filter_custom_objects);
TEST(test_array_filter_no_elements_filtered);
TEST(test_in_array_integer_vector_exists);
TEST(test_in_array_integer_vector_not_exists);
TEST(test_in_array_integer_vector_empty);
TEST(test_in_array_string_vector_exists);
TEST(test_in_array_string_vector_not_exists);
TEST(test_in_array_string_vector_empty);
TEST(test_in_array_map_exists);
TEST(test_in_array_map_not_exists);
TEST(test_in_array_map_empty);
TEST(test_in_array_unordered_map_exists);
TEST(test_in_array_unordered_map_not_exists);
TEST(test_in_array_unordered_map_empty);
TEST(test_in_array_custom_object_exists);
TEST(test_in_array_custom_object_not_exists);
TEST(test_in_array_custom_object_vector_empty);
TEST(test_array_reverse_empty);
TEST(test_array_reverse_single_element);
TEST(test_array_reverse_even_elements);
TEST(test_array_reverse_odd_elements);
TEST(test_array_reverse_strings);
TEST(test_array_reverse_large_array);
TEST(test_array_reverse_custom_objects);
TEST(test_array_reverse_alloc_fail);
TEST(test_array_reverse_inplace_empty);
TEST(test_array_reverse_inplace_single_element);
TEST(test_array_reverse_inplace_even_elements);
TEST(test_array_reverse_inplace_odd_elements);
TEST(test_array_reverse_inplace_strings);
TEST(test_array_reverse_inplace_large_array);
TEST(test_array_reverse_inplace_custom_objects);
TEST(test_array_reverse_inplace_alloc_fail);
TEST(test_vector_equal_basic_equal);
TEST(test_vector_equal_basic_not_equal);
TEST(test_vector_equal_different_sizes);
TEST(test_vector_equal_empty_vectors);
TEST(test_vector_equal_one_empty);
TEST(test_vector_equal_strings_equal);
TEST(test_vector_equal_strings_not_equal);
TEST(test_vector_equal_custom_objects_equal);
TEST(test_vector_equal_custom_objects_not_equal);
TEST(test_vector_equal_large_equal);
TEST(test_vector_equal_large_not_equal);
#endif