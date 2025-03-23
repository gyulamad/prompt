#pragma once

#include <tuple>

using namespace std;

namespace tools::containers {

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
    
}

#ifdef TEST

using namespace tools::containers;

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
#endif
