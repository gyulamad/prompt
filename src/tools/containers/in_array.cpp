#include "in_array.h"
#include <vector>
#include <map>
#include <unordered_map>
#include <string>

namespace tools::containers {

} // namespace tools::containers

#ifdef TEST

#include "../utils/Test.h"
#include "../utils/assert.hpp"

using namespace std;
using namespace tools::containers;

void test_in_array_integer_vector_exists() {
    vector<int> vec = {1, 2, 3, 4, 5};
    assert(tools::containers::in_array(3, vec) && "Integer exists in vector");
}

void test_in_array_integer_vector_not_exists() {
    vector<int> vec = {1, 2, 3, 4, 5};
    assert(!tools::containers::in_array(6, vec) && "Integer does not exist in vector");
}

void test_in_array_integer_vector_empty() {
    vector<int> vec;
    assert(!tools::containers::in_array(1, vec) && "Integer does not exist in empty vector");
}

void test_in_array_string_vector_exists() {
    vector<string> vec = {"apple", "banana", "cherry"};
    assert(tools::containers::in_array(string("banana"), vec) && "String exists in vector");
}

void test_in_array_string_vector_not_exists() {
    vector<string> vec = {"apple", "banana", "cherry"};
    assert(!tools::containers::in_array(string("grape"), vec) && "String does not exist in vector");
}

void test_in_array_string_vector_empty() {
    vector<string> vec;
    assert(!tools::containers::in_array(string("apple"), vec) && "String does not exist in empty vector");
}

void test_in_array_map_exists() {
    map<int, string> myMap = {{1, "one"}, {2, "two"}, {3, "three"}};
    assert(tools::containers::in_array(string("two"), myMap) && "Value exists in map");
}

void test_in_array_map_not_exists() {
    map<int, string> myMap = {{1, "one"}, {2, "two"}, {3, "three"}};
    assert(!tools::containers::in_array(string("four"), myMap) && "Value does not exist in map");
}

void test_in_array_map_empty() {
    map<int, string> myMap;
    assert(!tools::containers::in_array(string("one"), myMap) && "Value does not exist in empty map");
}

void test_in_array_unordered_map_exists() {
    unordered_map<int, string> myMap = {{1, "one"}, {2, "two"}, {3, "three"}};
    assert(tools::containers::in_array(string("two"), myMap) && "Value exists in unordered_map");
}

void test_in_array_unordered_map_not_exists() {
    unordered_map<int, string> myMap = {{1, "one"}, {2, "two"}, {3, "three"}};
    assert(!tools::containers::in_array(string("four"), myMap) && "Value does not exist in unordered_map");
}

void test_in_array_unordered_map_empty() {
    unordered_map<int, string> myMap;
    assert(!tools::containers::in_array(string("one"), myMap) && "Value does not exist in empty unordered_map");
}

void test_in_array_custom_object_exists() {
    struct Point {
        int x, y;
        bool operator==(const Point& other) const {
            return x == other.x && y == other.y;
        }
    };
    vector<Point> vec = {{1, 2}, {3, 4}, {5, 6}};
    assert(tools::containers::in_array(Point{3, 4}, vec) && "Custom object exists in vector");
}

void test_in_array_custom_object_not_exists() {
    struct Point {
        int x, y;
        bool operator==(const Point& other) const {
            return x == other.x && y == other.y;
        }
    };
    vector<Point> vec = {{1, 2}, {3, 4}, {5, 6}};
    assert(!tools::containers::in_array(Point{7, 8}, vec) && "Custom object does not exist in vector");
}

void test_in_array_custom_object_vector_empty() {
    struct Point {
        int x, y;
        // LCOV_EXCL_START
        bool operator==(const Point& other) const {
            return x == other.x && y == other.y;
        }
        // LCOV_EXCL_STOP
    };
    vector<Point> vec;
    assert(!tools::containers::in_array(Point{1, 2}, vec) && "Custom object does not exist in empty vector");
}

// Test registrations
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

#endif // TEST